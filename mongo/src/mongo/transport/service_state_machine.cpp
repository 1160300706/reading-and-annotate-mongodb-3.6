/**
 *    Copyright (C) 2017 MongoDB Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kNetwork

#include "mongo/platform/basic.h"

#include "mongo/transport/service_state_machine.h"

#include "mongo/config.h"
#include "mongo/db/client.h"
#include "mongo/db/dbmessage.h"
#include "mongo/db/stats/counters.h"
#include "mongo/stdx/memory.h"
#include "mongo/transport/message_compressor_manager.h"
#include "mongo/transport/service_entry_point.h"
#include "mongo/transport/session.h"
#include "mongo/transport/ticket.h"
#include "mongo/transport/transport_layer.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/concurrency/idle_thread_block.h"
#include "mongo/util/concurrency/thread_name.h"
#include "mongo/util/debug_util.h"
#include "mongo/util/exit.h"
#include "mongo/util/log.h"
#include "mongo/util/net/message.h"
#include "mongo/util/net/socket_exception.h"
#include "mongo/util/net/thread_idle_callback.h"
#include "mongo/util/quick_exit.h"

namespace mongo {
namespace {
// Set up proper headers for formatting an exhaust request, if we need to
bool setExhaustMessage(Message* m, const DbResponse& dbresponse) {
    MsgData::View header = dbresponse.response.header();
    QueryResult::View qr = header.view2ptr();
    long long cursorid = qr.getCursorId();

    if (!cursorid) {
        return false;
    }

    invariant(dbresponse.exhaustNS.size() && dbresponse.exhaustNS[0]);

    auto ns = dbresponse.exhaustNS;  // m->reset() will free this so we must cache a copy

    m->reset();

    // Rebuild out the response.
    BufBuilder b(512);
    b.appendNum(static_cast<int>(0) /* size set later in setLen() */);
    b.appendNum(header.getId());               // message id
    b.appendNum(header.getResponseToMsgId());  // in response to
    b.appendNum(static_cast<int>(dbGetMore));  // opCode is OP_GET_MORE
    b.appendNum(static_cast<int>(0));          // Must be ZERO (reserved)
    b.appendStr(ns);                           // Namespace
    b.appendNum(static_cast<int>(0));          // ntoreturn
    b.appendNum(cursorid);                     // cursor id from the OP_REPLY

    MsgData::View(b.buf()).setLen(b.len());
    m->setData(b.release());

    return true;
}

}  // namespace

using transport::ServiceExecutor;
using transport::TransportLayer;

/*
 * This class wraps up the logic for swapping/unswapping the Client during runNext().
 *
 * In debug builds this also ensures that only one thread is working on the SSM at once.
 */ 
/*
Breakpoint 1, mongo::ServiceStateMachine::ThreadGuard::ThreadGuard (this=0x7f60ef378e90, ssm=<optimized out>) at src/mongo/transport/service_state_machine.cpp:128
128                             log() << "yang test ......2.....ServiceStateMachine::ThreadGuard:" << _ssm->_threadName;
(gdb) bt
#0  mongo::ServiceStateMachine::ThreadGuard::ThreadGuard (this=0x7f60ef378e90, ssm=<optimized out>) at src/mongo/transport/service_state_machine.cpp:128
#1  0x00007f60fd72f791 in mongo::ServiceStateMachine::start (this=0x7f60ff8d3a90, ownershipModel=mongo::ServiceStateMachine::kOwned) at src/mongo/transport/service_state_machine.cpp:534
#2  0x00007f60fd72d2b1 in mongo::ServiceEntryPointImpl::startSession (this=<optimized out>, session=...) at src/mongo/transport/service_entry_point_impl.cpp:169
#3  0x00007f60fde02987 in operator() (peerSocket=..., ec=..., __closure=0x7f60ef379280) at src/mongo/transport/transport_layer_asio.cpp:334
#4  operator() (this=0x7f60ef379280) at src/third_party/asio-master/asio/include/asio/detail/bind_handler.hpp:308
#5  asio_handler_invoke<asio::detail::move_binder2<mongo::transport::TransportLayerASIO::_acceptConnection(mongo::transport::TransportLayerASIO::GenericAcceptor&)::<lambda(const std::error_code&, mongo::transport::GenericSocket)>, std::error_code, asio::basic_stream_socket<asio::generic::stream_protocol> > > (function=...) at src/third_party/asio-master/asio/include/asio/handler_invoke_hook.hpp:68
#6  invoke<asio::detail::move_binder2<mongo::transport::TransportLayerASIO::_acceptConnection(mongo::transport::TransportLayerASIO::GenericAcceptor&)::<lambda(const std::error_code&, mongo::transport::GenericSocket)>, std::error_code, asio::basic_stream_socket<asio::generic::stream_protocol> >, mongo::transport::TransportLayerASIO::_acceptConnection(mongo::transport::TransportLayerASIO::GenericAcceptor&)::<lambda(const std::error_code&, mongo::transport::GenericSocket)> > (context=..., function=...) at src/third_party/asio-master/asio/include/asio/detail/handler_invoke_helpers.hpp:37
#7  complete<asio::detail::move_binder2<mongo::transport::TransportLayerASIO::_acceptConnection(mongo::transport::TransportLayerASIO::GenericAcceptor&)::<lambda(const std::error_code&, mongo::transport::GenericSocket)>, std::error_code, asio::basic_stream_socket<asio::generic::stream_protocol> > > (this=<synthetic pointer>, handler=..., function=...) at src/third_party/asio-master/asio/include/asio/detail/handler_work.hpp:81
#8  asio::detail::reactive_socket_move_accept_op<asio::generic::stream_protocol, mongo::transport::TransportLayerASIO::_acceptConnection(mongo::transport::TransportLayerASIO::GenericAcceptor&)::<lambda(const std::error_code&, mongo::transport::GenericSocket)> >::do_complete(void *, asio::detail::operation *, const asio::error_code &, std::size_t) (owner=<optimized out>, base=<optimized out>)
    at src/third_party/asio-master/asio/include/asio/detail/reactive_socket_accept_op.hpp:201
#9  0x00007f60fde0ccd9 in complete (bytes_transferred=<optimized out>, ec=..., owner=0x7f60ff58b100, this=<optimized out>) at src/third_party/asio-master/asio/include/asio/detail/scheduler_operation.hpp:39
#10 asio::detail::scheduler::do_run_one (this=this@entry=0x7f60ff58b100, lock=..., this_thread=..., ec=...) at src/third_party/asio-master/asio/include/asio/detail/impl/scheduler.ipp:400
#11 0x00007f60fde0cf21 in asio::detail::scheduler::run (this=0x7f60ff58b100, ec=...) at src/third_party/asio-master/asio/include/asio/detail/impl/scheduler.ipp:153
#12 0x00007f60fde1716e in asio::io_context::run (this=0x7f60ff7a8a90) at src/third_party/asio-master/asio/include/asio/impl/io_context.ipp:61
#13 0x00007f60fde0071e in mongo::transport::TransportLayerASIO::<lambda()>::operator()(void) const (__closure=0x7f60ff6ac2e8) at src/mongo/transport/transport_layer_asio.cpp:249
#14 0x00007f60fc7428f0 in std::execute_native_thread_routine (__p=<optimized out>) at ../../../.././libstdc++-v3/src/c++11/thread.cc:84
#15 0x00007f60fbf5ee25 in start_thread () from /lib64/libpthread.so.0
#16 0x00007f60fbc8c34d in clone () from /lib64/libc.so.6
(gdb) c
*/
//ThreadGuard ��صĶ�������
class ServiceStateMachine::ThreadGuard {
    ThreadGuard(ThreadGuard&) = delete;
    ThreadGuard& operator=(ThreadGuard&) = delete;

public:
    explicit ThreadGuard(ServiceStateMachine* ssm) : _ssm{ssm} {
		//���ServiceStateMachine._owned=kUnowned,��ServiceStateMachine._owned��ֵΪkOwned
        auto owned = _ssm->_owned.compareAndSwap(Ownership::kUnowned, Ownership::kOwned);
        if (owned == Ownership::kStatic) { //sync�߳�ģʽ
            dassert(haveClient());
            dassert(Client::getCurrent() == _ssm->_dbClientPtr);
            _haveTakenOwnership = true;
            return;
        }

		//adaptive async�߳�ģʽ�������ģʽ

#ifdef MONGO_CONFIG_DEBUG_BUILD
        invariant(owned == Ownership::kUnowned);
        _ssm->_owningThread.store(stdx::this_thread::get_id());
#endif

        // Set up the thread name
        auto oldThreadName = getThreadName(); //���߳���ǰ���߳�������ʱ��������
        if (oldThreadName != _ssm->_threadName) {
			//��¼��֮ǰ���߳���
            _ssm->_oldThreadName = getThreadName().toString();
			log() << "yang test ...........ServiceStateMachine::ThreadGuard:" << _ssm->_oldThreadName;
            setThreadName(_ssm->_threadName); //�ѵ�ǰ�̸߳���Ϊ_threadName
			//sleep(60);
			log() << "yang test ......2.....ServiceStateMachine::ThreadGuard:" << _ssm->_threadName;
        }

        // Swap the current Client so calls to cc() work as expected
        Client::setCurrent(std::move(_ssm->_dbClient));
        _haveTakenOwnership = true;
    }

    // Constructing from a moved ThreadGuard invalidates the other thread guard.
    ThreadGuard(ThreadGuard&& other)
        : _ssm(other._ssm), _haveTakenOwnership(other._haveTakenOwnership) {
        other._haveTakenOwnership = false;
    }

    ThreadGuard& operator=(ThreadGuard&& other) {
        if (this != &other) {
            _ssm = other._ssm;
            _haveTakenOwnership = other._haveTakenOwnership;
            other._haveTakenOwnership = false;
        }
        return *this;
    };

    ThreadGuard() = delete;

    ~ThreadGuard() {
        if (_haveTakenOwnership)
            release();
    }

    explicit operator bool() const {
#ifdef MONGO_CONFIG_DEBUG_BUILD
        if (_haveTakenOwnership) {
            invariant(_ssm->_owned.load() != Ownership::kUnowned);
            invariant(_ssm->_owningThread.load() == stdx::this_thread::get_id());
            return true;
        } else {
            return false;
        }
#else
        return _haveTakenOwnership;
#endif
    }

	//ServiceStateMachine::_scheduleNextWithGuard
    void markStaticOwnership() {
        dassert(static_cast<bool>(*this));
        _ssm->_owned.store(Ownership::kStatic);
    }

    void release() {
        auto owned = _ssm->_owned.load();

#ifdef MONGO_CONFIG_DEBUG_BUILD
        dassert(_haveTakenOwnership);
        dassert(owned != Ownership::kUnowned);
        dassert(_ssm->_owningThread.load() == stdx::this_thread::get_id());
#endif
        if (owned != Ownership::kStatic) {//async�̳߳�ģʽ����if����
            if (haveClient()) {
                _ssm->_dbClient = Client::releaseCurrent();
            }

            if (!_ssm->_oldThreadName.empty()) {
				
                setThreadName(_ssm->_oldThreadName); //�ָ������߳���
            }
        }

        // If the session has ended, then it's unsafe to do anything but call the cleanup hook.
        if (_ssm->state() == State::Ended) {
            // The cleanup hook gets moved out of _ssm->_cleanupHook so that it can only be called
            // once.
            auto cleanupHook = std::move(_ssm->_cleanupHook);
            if (cleanupHook)
                cleanupHook();

            // It's very important that the Guard returns here and that the SSM's state does not
            // get modified in any way after the cleanup hook is called.
            return;
        }

        _haveTakenOwnership = false;
        // If owned != Ownership::kOwned here then it can only equal Ownership::kStatic and we
        // should just return
        if (owned == Ownership::kOwned) {
            _ssm->_owned.store(Ownership::kUnowned);
        }
    }

private:
    ServiceStateMachine* _ssm;
    bool _haveTakenOwnership = false;
};

//�����µ�worker-n�߳�ServiceExecutorAdaptive::_startWorkerThread->_workerThreadRoutine   conn�̴߳�����ServiceStateMachine::create 

//TransportLayerASIO::_acceptConnection->ServiceEntryPointImpl::startSession->ServiceStateMachine::create 

//ServiceEntryPointImpl::startSession�е��ã�sessionĬ�϶�ӦASIOSession
std::shared_ptr<ServiceStateMachine> ServiceStateMachine::create(ServiceContext* svcContext,
                                                                 transport::SessionHandle session,
                                                                 transport::Mode transportMode) {
    return std::make_shared<ServiceStateMachine>(svcContext, std::move(session), transportMode);
}

//ServiceStateMachine::create����   �����������߳���
ServiceStateMachine::ServiceStateMachine(ServiceContext* svcContext,
                                         transport::SessionHandle session,
                                         transport::Mode transportMode)
    : _state{State::Created},
      _sep{svcContext->getServiceEntryPoint()},
      _transportMode(transportMode),
      _serviceContext(svcContext),
      _sessionHandle(session),
      _dbClient{svcContext->makeClient("conn", std::move(session))},
      _dbClientPtr{_dbClient.get()},
      //������Ч��ServiceStateMachine::ThreadGuard
      _threadName{str::stream() << "conn-yang" << _session()->id()} {} //�߳���

const transport::SessionHandle& ServiceStateMachine::_session() const {
	//�ÿͻ���������Ϣ�ڸýṹ�У�Ҳ����ASIOSession
    return _sessionHandle;
}


/*
#0  mongo::ServiceStateMachine::_processMessage (this=this@entry=0x7f228ce66890, guard=...) at src/mongo/transport/service_state_machine.cpp:345
#1  0x00007f2285357c5f in mongo::ServiceStateMachine::_runNextInGuard (this=0x7f228ce66890, guard=...) at src/mongo/transport/service_state_machine.cpp:424
#2  0x00007f228535b69e in operator() (__closure=0x7f228cedd540) at src/mongo/transport/service_state_machine.cpp:463
#3  std::_Function_handler<void(), mongo::ServiceStateMachine::_scheduleNextWithGuard(mongo::ServiceStateMachine::ThreadGuard, mongo::transport::ServiceExecutor::ScheduleFlags, mongo::ServiceStateMachine::Ownership)::<lambda()> >::_M_invoke(const std::_Any_data &) (__functor=...) at /usr/local/include/c++/5.4.0/functional:1871
#4  0x00007f2286297c12 in operator() (this=0x7f22847a1550) at /usr/local/include/c++/5.4.0/functional:2267
#5  mongo::transport::ServiceExecutorSynchronous::schedule(std::function<void ()>, mongo::transport::ServiceExecutor::ScheduleFlags) (this=this@entry=0x7f2289601480, task=..., 
    flags=flags@entry=mongo::transport::ServiceExecutor::kMayRecurse) at src/mongo/transport/service_executor_synchronous.cpp:125
#6  0x00007f228535685d in mongo::ServiceStateMachine::_scheduleNextWithGuard (this=this@entry=0x7f228ce66890, guard=..., flags=flags@entry=mongo::transport::ServiceExecutor::kMayRecurse, 
    ownershipModel=ownershipModel@entry=mongo::ServiceStateMachine::kOwned) at src/mongo/transport/service_state_machine.cpp:467
#7  0x00007f22853591f1 in mongo::ServiceStateMachine::_sourceCallback (this=this@entry=0x7f228ce66890, status=...) at src/mongo/transport/service_state_machine.cpp:292
#8  0x00007f2285359deb in mongo::ServiceStateMachine::_sourceMessage (this=this@entry=0x7f228ce66890, guard=...) at src/mongo/transport/service_state_machine.cpp:251
#9  0x00007f2285357cf1 in mongo::ServiceStateMachine::_runNextInGuard (this=0x7f228ce66890, guard=...) at src/mongo/transport/service_state_machine.cpp:421
#10 0x00007f228535b69e in operator() (__closure=0x7f228cedd500) at src/mongo/transport/service_state_machine.cpp:463
#11 std::_Function_handler<void(), mongo::ServiceStateMachine::_scheduleNextWithGuard(mongo::ServiceStateMachine::ThreadGuard, mongo::transport::ServiceExecutor::ScheduleFlags, mongo::ServiceStateMachine::Ownership)::<lambda()> >::_M_invoke(const std::_Any_data &) (__functor=...) at /usr/local/include/c++/5.4.0/functional:1871
#12 0x00007f2286298175 in operator() (this=<optimized out>) at /usr/local/include/c++/5.4.0/functional:2267
#13 operator() (__closure=0x7f228ce81410) at src/mongo/transport/service_executor_synchronous.cpp:142
#14 std::_Function_handler<void(), mongo::transport::ServiceExecutorSynchronous::schedule(mongo::transport::ServiceExecutor::Task, mongo::transport::ServiceExecutor::ScheduleFlags)::<lambda()> >::_M_invoke(const std::_Any_data &) (
    __functor=...) at /usr/local/include/c++/5.4.0/functional:1871
#15 0x00007f22867e7d44 in operator() (this=<optimized out>) at /usr/local/include/c++/5.4.0/functional:2267
#16 mongo::(anonymous namespace)::runFunc (ctx=0x7f228cedd0a0) at src/mongo/transport/service_entry_point_utils.cpp:55
#17 0x00007f22834bce25 in start_thread () from /lib64/libpthread.so.0
#18 0x00007f22831ea34d in clone () from /lib64/libc.so.6
*/ //epoll�첽����ʱ�䴥����һ���������ݵ���   
void ServiceStateMachine::_sourceMessage(ThreadGuard guard) {
    invariant(_inMessage.empty());
	//TransportLayerASIO::sourceMessage  TransportLayerASIO::ASIOSession  �����wait asio���ȡ���ݷ���_inMessage
	//ServiceStateMachine::_sourceMessage->Session::sourceMessage->TransportLayerASIO::sourceMessage
    auto ticket = _session()->sourceMessage(&_inMessage); 

    _state.store(State::SourceWait); //�ȴ���ȡЭ��ջ�е�����
    guard.release();
	//�߳�ģ��Ĭ��ͬ����ʽ��Ҳ����һ������һ���߳�
    if (_transportMode == transport::Mode::kSynchronous) {
        _sourceCallback([this](auto ticket) {
            MONGO_IDLE_THREAD_BLOCK;
			//TransportLayerASIO::wait  
            return _session()->getTransportLayer()->wait(std::move(ticket));
        }(std::move(ticket))); 
    } else if (_transportMode == transport::Mode::kAsynchronous) {
    	//TransportLayerASIO::asyncWait
        _session()->getTransportLayer()->asyncWait( 
            std::move(ticket), [this](Status status) { _sourceCallback(status); });
    }
}

//��������
void ServiceStateMachine::_sinkMessage(ThreadGuard guard, Message toSink) {
    // Sink our response to the client
    auto ticket = _session()->sinkMessage(toSink);

    _state.store(State::SinkWait);
    guard.release();

    if (_transportMode == transport::Mode::kSynchronous) {
        _sinkCallback(_session()->getTransportLayer()->wait(std::move(ticket)));
    } else if (_transportMode == transport::Mode::kAsynchronous) {
        _session()->getTransportLayer()->asyncWait(
            std::move(ticket), [this](Status status) { _sinkCallback(status); });
    }
}

//mongos  TransportLayerASIO::asyncWait
//ServiceStateMachine::_sourceMessage��epoll_wait���أ�Ҳ����Э��ջ�������Ѿ������û�̬�ռ�
void ServiceStateMachine::_sourceCallback(Status status) {
    // The first thing to do is create a ThreadGuard which will take ownership of the SSM in this
    // thread.
    ThreadGuard guard(this); 

    // Make sure we just called sourceMessage();
    dassert(state() == State::SourceWait);
    auto remote = _session()->remote(); //��ȡ�ͻ�����Ϣ

    if (status.isOK()) {
        _state.store(State::Process);

        // Since we know that we're going to process a message, call scheduleNext() immediately
        // to schedule the call to processMessage() on the serviceExecutor (or just unwind the
        // stack)

        // If this callback doesn't own the ThreadGuard, then we're being called recursively,
        // and the executor shouldn't start a new thread to process the message - it can use this
        // one just after this returns.
        return _scheduleNextWithGuard(std::move(guard), ServiceExecutor::kMayRecurse);
    } else if (ErrorCodes::isInterruption(status.code()) ||
               ErrorCodes::isNetworkError(status.code())) {
        LOG(2) << "Session from " << remote << " encountered a network error during SourceMessage";
        _state.store(State::EndSession);
    } else if (status == TransportLayer::TicketSessionClosedStatus) {
        // Our session may have been closed internally.
        LOG(2) << "Session from " << remote << " was closed internally during SourceMessage";
        _state.store(State::EndSession);
    } else {
        log() << "Error receiving request from client: " << status << ". Ending connection from "
              << remote << " (connection id: " << _session()->id() << ")";
        _state.store(State::EndSession);
    }

    // There was an error receiving a message from the client and we've already printed the error
    // so call runNextInGuard() to clean up the session without waiting.
    _runNextInGuard(std::move(guard));
}

void ServiceStateMachine::_sinkCallback(Status status) {
    // The first thing to do is create a ThreadGuard which will take ownership of the SSM in this
    // thread.
    ThreadGuard guard(this);

    dassert(state() == State::SinkWait);

    // If there was an error sinking the message to the client, then we should print an error and
    // end the session. No need to unwind the stack, so this will runNextInGuard() and return.
    //
    // Otherwise, update the current state depending on whether we're in exhaust or not, and call
    // scheduleNext() to unwind the stack and do the next step.
    if (!status.isOK()) {
        log() << "Error sending response to client: " << status << ". Ending connection from "
              << _session()->remote() << " (connection id: " << _session()->id() << ")";
        _state.store(State::EndSession);
        return _runNextInGuard(std::move(guard));
    } else if (_inExhaust) { //3.6.1�汾���������㣬��Ϊexhaust����û������
        _state.store(State::Process);
    } else { //��������ʼ�ս���÷�֧
        _state.store(State::Source);
    }

    return _scheduleNextWithGuard(std::move(guard),
                                  ServiceExecutor::kDeferredTask |
                                      ServiceExecutor::kMayYieldBeforeSchedule);
}

/*
#0  mongo::ServiceStateMachine::_processMessage (this=this@entry=0x7f228ce66890, guard=...) at src/mongo/transport/service_state_machine.cpp:345
#1  0x00007f2285357c5f in mongo::ServiceStateMachine::_runNextInGuard (this=0x7f228ce66890, guard=...) at src/mongo/transport/service_state_machine.cpp:424
#2  0x00007f228535b69e in operator() (__closure=0x7f228cedd540) at src/mongo/transport/service_state_machine.cpp:463
#3  std::_Function_handler<void(), mongo::ServiceStateMachine::_scheduleNextWithGuard(mongo::ServiceStateMachine::ThreadGuard, mongo::transport::ServiceExecutor::ScheduleFlags, mongo::ServiceStateMachine::Ownership)::<lambda()> >::_M_invoke(const std::_Any_data &) (__functor=...) at /usr/local/include/c++/5.4.0/functional:1871
#4  0x00007f2286297c12 in operator() (this=0x7f22847a1550) at /usr/local/include/c++/5.4.0/functional:2267
#5  mongo::transport::ServiceExecutorSynchronous::schedule(std::function<void ()>, mongo::transport::ServiceExecutor::ScheduleFlags) (this=this@entry=0x7f2289601480, task=..., 
    flags=flags@entry=mongo::transport::ServiceExecutor::kMayRecurse) at src/mongo/transport/service_executor_synchronous.cpp:125
#6  0x00007f228535685d in mongo::ServiceStateMachine::_scheduleNextWithGuard (this=this@entry=0x7f228ce66890, guard=..., flags=flags@entry=mongo::transport::ServiceExecutor::kMayRecurse, 
    ownershipModel=ownershipModel@entry=mongo::ServiceStateMachine::kOwned) at src/mongo/transport/service_state_machine.cpp:467
#7  0x00007f22853591f1 in mongo::ServiceStateMachine::_sourceCallback (this=this@entry=0x7f228ce66890, status=...) at src/mongo/transport/service_state_machine.cpp:292
#8  0x00007f2285359deb in mongo::ServiceStateMachine::_sourceMessage (this=this@entry=0x7f228ce66890, guard=...) at src/mongo/transport/service_state_machine.cpp:251
#9  0x00007f2285357cf1 in mongo::ServiceStateMachine::_runNextInGuard (this=0x7f228ce66890, guard=...) at src/mongo/transport/service_state_machine.cpp:421
#10 0x00007f228535b69e in operator() (__closure=0x7f228cedd500) at src/mongo/transport/service_state_machine.cpp:463
#11 std::_Function_handler<void(), mongo::ServiceStateMachine::_scheduleNextWithGuard(mongo::ServiceStateMachine::ThreadGuard, mongo::transport::ServiceExecutor::ScheduleFlags, mongo::ServiceStateMachine::Ownership)::<lambda()> >::_M_invoke(const std::_Any_data &) (__functor=...) at /usr/local/include/c++/5.4.0/functional:1871
#12 0x00007f2286298175 in operator() (this=<optimized out>) at /usr/local/include/c++/5.4.0/functional:2267
#13 operator() (__closure=0x7f228ce81410) at src/mongo/transport/service_executor_synchronous.cpp:142
#14 std::_Function_handler<void(), mongo::transport::ServiceExecutorSynchronous::schedule(mongo::transport::ServiceExecutor::Task, mongo::transport::ServiceExecutor::ScheduleFlags)::<lambda()> >::_M_invoke(const std::_Any_data &) (
    __functor=...) at /usr/local/include/c++/5.4.0/functional:1871
#15 0x00007f22867e7d44 in operator() (this=<optimized out>) at /usr/local/include/c++/5.4.0/functional:2267
#16 mongo::(anonymous namespace)::runFunc (ctx=0x7f228cedd0a0) at src/mongo/transport/service_entry_point_utils.cpp:55
#17 0x00007f22834bce25 in start_thread () from /lib64/libpthread.so.0
#18 0x00007f22831ea34d in clone () from /lib64/libc.so.6
*/
//��Ϣ�������ߵ�����
void ServiceStateMachine::_processMessage(ThreadGuard guard) {
    invariant(!_inMessage.empty());
	//log() << "	yang test ...........	_processMessage ";

	//��ȡ��MessageCompressorManager
    auto& compressorMgr = MessageCompressorManager::forSession(_session());

    _compressorId = boost::none;
    if (_inMessage.operation() == dbCompressed) { //
        MessageCompressorId compressorId;
        auto swm = compressorMgr.decompressMessage(_inMessage, &compressorId);
        uassertStatusOK(swm.getStatus());
        _inMessage = swm.getValue();
        _compressorId = compressorId;
    }

    networkCounter.hitLogicalIn(_inMessage.size());

    // Pass sourced Message to handler to generate response.
    auto opCtx = Client::getCurrent()->makeOperationContext();

    // The handleRequest is implemented in a subclass for mongod/mongos and actually all the
    // database work for this request.
    //ServiceEntryPointMongod::handleRequest  ServiceEntryPointMongos::handleRequest������
    DbResponse dbresponse = _sep->handleRequest(opCtx.get(), _inMessage);

    // opCtx must be destroyed here so that the operation cannot show
    // up in currentOp results after the response reaches the client
    opCtx.reset();

    // Format our response, if we have one
    Message& toSink = dbresponse.response;
    if (!toSink.empty()) { //Ӧ����
        invariant(!OpMsg::isFlagSet(_inMessage, OpMsg::kMoreToCome));
        toSink.header().setId(nextMessageId());
        toSink.header().setResponseToMsgId(_inMessage.header().getId());

        // If this is an exhaust cursor, don't source more Messages
        //3.6.1�汾��Exhaust��û�������������Բ������_inExhaust = true;
        if (dbresponse.exhaustNS.size() > 0 && setExhaustMessage(&_inMessage, dbresponse)) {
            _inExhaust = true;  
        } else {
            _inExhaust = false;
            _inMessage.reset();
        }

        networkCounter.hitLogicalOut(toSink.size());

        if (_compressorId) {
            auto swm = compressorMgr.compressMessage(toSink, &_compressorId.value());
            uassertStatusOK(swm.getStatus());
            toSink = swm.getValue();
        }
        _sinkMessage(std::move(guard), std::move(toSink));

    } else {
        _state.store(State::Source);
        _inMessage.reset();
        return _scheduleNextWithGuard(std::move(guard), ServiceExecutor::kDeferredTask);
    }
}

void ServiceStateMachine::runNext() {
    return _runNextInGuard(ThreadGuard(this));
}

//ServiceStateMachine::_scheduleNextWithGuard  ServiceStateMachine::_sourceCallback��ִ��
void ServiceStateMachine::_runNextInGuard(ThreadGuard guard) {
    auto curState = state();
    dassert(curState != State::Ended);

    // If this is the first run of the SSM, then update its state to Source
    if (curState == State::Created) { //��ʼ״̬
        curState = State::Source;
        _state.store(curState);
    }

    // Make sure the current Client got set correctly
    dassert(Client::getCurrent() == _dbClientPtr);
	/*
	    enum class State {
        Created,     // The session has been created, but no operations have been performed yet
        Source,      // Request a new Message from the network to handle
        SourceWait,  // Wait for the new Message to arrive from the network
        Process,     // Run the Message through the database
        SinkWait,    // Wait for the database result to be sent by the network
        EndSession,  // End the session - the ServiceStateMachine will be invalid after this
        Ended        // The session has ended. It is illegal to call any method besides
                     // state() if this is the current state.
    };
	*/
	/*
	���Ǹ�״̬�����ں�epoll�������������ݵ�������ִ��_sourceMessage��_sourceMessage�е���TransportLayerASIO::wait��
	��ȡЭ��ջ�����ݣ���ȡ���ػᣬ��_sourceMessage->ServiceStateMachine::_sourceCallback�а�״̬��ΪState::Process,��˼��
	���Ը���mongodЭ��
	*/
    try {
        switch (curState) { 
            case State::Source: //��ʼȥʹ��epoll������ȡ���ݽ���
                _sourceMessage(std::move(guard));
                break;
            case State::Process:
                _processMessage(std::move(guard));
                break;
            case State::EndSession:
                _cleanupSession(std::move(guard));
                break;
            default:
                MONGO_UNREACHABLE;
        }

        return;
    } catch (const DBException& e) {
        // must be right above std::exception to avoid catching subclasses
        log() << "DBException handling request, closing client connection: " << redact(e);
    } catch (const std::exception& e) {
        error() << "Uncaught std::exception: " << e.what() << ", terminating";
        quickExit(EXIT_UNCAUGHT);
    }

    if (!guard) {
        guard = ThreadGuard(this);
    }
    _state.store(State::EndSession);
    _cleanupSession(std::move(guard));
}

//ServiceEntryPointImpl::startSession��ִ��  ����
void ServiceStateMachine::start(Ownership ownershipModel) {
    _scheduleNextWithGuard( 
		//��ʱ�Եı�Ϊconn�߳��� //�̸߳���Ҳ��������  "conn-"�߳���  
		//ServiceStateMachine::start�е�ThreadGuard(this)���߳�����ֵΪconn-x��������̸߳���Ϊconn-x
        ThreadGuard(this), transport::ServiceExecutor::kEmptyFlags, ownershipModel);
}

//�����ServiceStateMachine::start(�����ӵ���)  ServiceStateMachine::_sourceCallback(���������ݵ�������)��ִ��
void ServiceStateMachine::_scheduleNextWithGuard(ThreadGuard guard,
                                                 transport::ServiceExecutor::ScheduleFlags flags,
                                                 Ownership ownershipModel) {
	//func��ServiceExecutorSynchronous::schedule��ִ��
    auto func = [ ssm = shared_from_this(), ownershipModel ] {
		//ServiceStateMachine::start�е�ThreadGuard(this)���߳�����ֵΪconn-x
        ThreadGuard guard(ssm.get());  //��ӦThreadGuard& operator=(ThreadGuard&& other)
        if (ownershipModel == Ownership::kStatic) //˵����sync mode
            guard.markStaticOwnership();
		//log() << "yang test  ServiceStateMachine::_scheduleNextWithGuard 22";
		//��Ӧ:ServiceStateMachine::_runNextInGuard
		////ServiceExecutorAdaptive::schedule(adaptive)   ServiceExecutorSynchronous::schedule(synchronous)��ִ��
        ssm->_runNextInGuard(std::move(guard)); //������conn�߳�����Ҫִ�е�task
    };

	//log() << "yang test  ServiceStateMachine::_scheduleNextWithGuard 11";

	//��ServiceStateMachine::start�е�ThreadGuard(this)��Ӧ
    guard.release();
	//ServiceExecutorAdaptive::schedule(adaptive)   ServiceExecutorSynchronous::schedule(synchronous)
	//��һ�ν���ú�����ʱ���������洴�����߳�
    Status status = _serviceContext->getServiceExecutor()->schedule(std::move(func), flags);
    if (status.isOK()) {
        return;
    }

    // We've had an error, reacquire the ThreadGuard and destroy the SSM
    ThreadGuard terminateGuard(this);  //��ӦThreadGuard& operator=(ThreadGuard&& other)

    // The service executor failed to schedule the task. This could for example be that we failed
    // to start a worker thread. Terminate this connection to leave the system in a valid state.
    _terminateAndLogIfError(status);
    _cleanupSession(std::move(terminateGuard));
}

void ServiceStateMachine::terminate() {
    if (state() == State::Ended)
        return;

    _session()->getTransportLayer()->end(_session());
}

void ServiceStateMachine::terminateIfTagsDontMatch(transport::Session::TagMask tags) {
    if (state() == State::Ended)
        return;

    auto sessionTags = _session()->getTags();

    // If terminateIfTagsDontMatch gets called when we still are 'pending' where no tags have been
    // set, then skip the termination check.
    if ((sessionTags & tags) || (sessionTags & transport::Session::kPending)) {
        log() << "Skip closing connection for connection # " << _session()->id();
        return;
    }

    terminate();
}

void ServiceStateMachine::setCleanupHook(stdx::function<void()> hook) {
    invariant(state() == State::Created);
    _cleanupHook = std::move(hook);
}

ServiceStateMachine::State ServiceStateMachine::state() {
    return _state.load();
}

void ServiceStateMachine::_terminateAndLogIfError(Status status) {
    if (!status.isOK()) {
        warning(logger::LogComponent::kExecutor) << "Terminating session due to error: " << status;
        terminate();
    }
}

void ServiceStateMachine::_cleanupSession(ThreadGuard guard) {
    _state.store(State::Ended);

    _inMessage.reset();

    // By ignoring the return value of Client::releaseCurrent() we destroy the session.
    // _dbClient is now nullptr and _dbClientPtr is invalid and should never be accessed.
    Client::releaseCurrent();
}

}  // namespace mongo
