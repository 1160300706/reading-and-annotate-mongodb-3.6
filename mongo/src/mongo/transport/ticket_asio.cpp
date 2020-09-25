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

#include "mongo/base/system_error.h"
#include "mongo/db/stats/counters.h"
#include "mongo/transport/asio_utils.h"
#include "mongo/transport/ticket_asio.h"
#include "mongo/util/log.h"

#include "mongo/transport/session_asio.h"

namespace mongo {
namespace transport {
namespace {
	//mongoЭ��ͷ������  TransportLayerASIO::ASIOSourceTicket::fillImpl
constexpr auto kHeaderSize = sizeof(MSGHEADER::Value);

}  // namespace

//��ȡ��ASIOTicket��session��Ϣ
std::shared_ptr<TransportLayerASIO::ASIOSession> TransportLayerASIO::ASIOTicket::getSession() {
    auto session = _session.lock();
    if (!session || !session->isOpen()) {
        finishFill(Ticket::SessionClosedStatus);
        return nullptr;
    }
    return session;
}

//ServiceStateMachine::_sourceMessage���Կ�����kSynchronousģʽΪtrue��adaptiveģʽΪfalse
bool TransportLayerASIO::ASIOTicket::isSync() const {
    return _fillSync;
}

//��ʼ������ASIOTicket��
TransportLayerASIO::ASIOTicket::ASIOTicket(const ASIOSessionHandle& session, Date_t expiration)
    : _session(session), _sessionId(session->id()), _expiration(expiration) {}

////��ʼ������ASIOSourceTicket��
TransportLayerASIO::ASIOSourceTicket::ASIOSourceTicket(const ASIOSessionHandle& session,
                                                       Date_t expiration,
                                                       Message* msg)
    : ASIOTicket(session, expiration), _target(msg) {}

////��ʼ������ASIOSinkTicket��
TransportLayerASIO::ASIOSinkTicket::ASIOSinkTicket(const ASIOSessionHandle& session,
                                                   Date_t expiration,
                                                   const Message& msg)
    : ASIOTicket(session, expiration), _msgToSend(msg) {}
//TransportLayerASIO::ASIOSourceTicket::_headerCallback
//_headerCallback��header��ȡ�����headerͷ����ȡ����Ӧ��msg���ȣ�Ȼ��ʼbody���ִ���
void TransportLayerASIO::ASIOSourceTicket::_bodyCallback(const std::error_code& ec, size_t size) {
    if (ec) {
        finishFill(errorCodeToStatus(ec));
        return;
    }

	//bufferת�浽_target��
    _target->setData(std::move(_buffer));
	//����ͳ��
    networkCounter.hitPhysicalIn(_target->size());
	//TransportLayerASIO::ASIOTicket::finishFill  
    finishFill(Status::OK()); //�������ݶ���󣬿�ʼ��һ�׶εĴ���  
    //���Ķ�ȡ������һ�׶ξ��Ǳ������ݴ�����ʼ��ServiceStateMachine::_sourceCallback
}

//TransportLayerASIO::ASIOSourceTicket::fillImpl
//fillImpl��Э��ջ��ȡ�����ݺ�ʼ������������������㹻ͷ���ֶΣ������finishFill����һ�����������������read����������
//��ȡ��mongodb headerͷ����Ϣ��Ļص�����
void TransportLayerASIO::ASIOSourceTicket::_headerCallback(const std::error_code& ec, size_t size) {
    if (ec) {
        finishFill(errorCodeToStatus(ec));
        return;
    }
	//��ȡsession��Ϣ
    auto session = getSession();
    if (!session)
        return;
	//��_buffer�л�ȡͷ����Ϣ
    MSGHEADER::View headerView(_buffer.get());
	//��ȡmessage����
    auto msgLen = static_cast<size_t>(headerView.getMessageLength());
	//����̫С����̫��ֱ�ӱ���
    if (msgLen < kHeaderSize || msgLen > MaxMessageSizeBytes) {
        StringBuilder sb;
        sb << "recv(): message msgLen " << msgLen << " is invalid. "
           << "Min " << kHeaderSize << " Max: " << MaxMessageSizeBytes;
        const auto str = sb.str();
        LOG(0) << str;
        finishFill(Status(ErrorCodes::ProtocolError, str));
        return;
    }

	//˵�����ݲ���Ҳ��ȡ�����ˣ�һ��������mongo���Ķ�ȡ���,Ҳ���Ǳ���ֻ����ͷ����û�а����Э������
    if (msgLen == size) {
        finishFill(Status::OK());
        return;
    }

	//���ݻ�����һ��mongoЭ�鱨�ģ�������ȡbody�����ֽڵ����ݣ���ȡ��Ϻ�ʼbody����
    _buffer.realloc(msgLen); //ע��������realloc����֤ͷ����body��ͬһ��buffer��
    MsgData::View msgView(_buffer.get());


	//��ȡ����body TransportLayerASIO::ASIOSession::read
    session->read(isSync(),
      //���ݶ�ȡ����buffer				
      asio::buffer(msgView.data(), msgView.dataLen()),
      //��ȡ�ɹ���Ļص�����
      [this](const std::error_code& ec, size_t size) { _bodyCallback(ec, size); });
}

/*
//accept��Ӧ��״̬�������������
//TransportLayerASIO::_acceptConnection->basic_socket_acceptor::async_accept
//->start_accept_op->epoll_reactor::post_immediate_completion

//��ͨread write��Ӧ��״̬�������������
//mongodb��ServiceExecutorAdaptive::schedule����->io_context::post(ASIO_MOVE_ARG(CompletionHandler) handler)
//->scheduler::post_immediate_completion
//mongodb��ServiceExecutorAdaptive::schedule����->io_context::dispatch(ASIO_MOVE_ARG(CompletionHandler) handler)
//->scheduler::do_dispatch

//��ͨ��дread write��Ӧ��״̬�������������
//ServiceExecutorAdaptive::_workerThreadRoutine->io_context::run_for->scheduler::wait_one
//->scheduler::do_wait_one����
//mongodb��ServiceExecutorAdaptive::_workerThreadRoutine->io_context::run_one_for
//->io_context::run_one_until->schedule::wait_one
		|
		|1.�Ƚ���״̬���������(Ҳ����mongodb��TransportLayerASIO._workerIOContext  TransportLayerASIO._acceptorIOContext��ص�����)
		|2.��ִ�в���1��Ӧ����������������յ���TransportLayerASIO::_acceptConnection��TransportLayerASIO::ASIOSourceTicket::fillImpl��
		|  TransportLayerASIO::ASIOSinkTicket::fillImpl���������Ӵ������ݶ�д�¼�epollע��(�����ͷ����)
		|
	    \|/
//accept��Ӧ��������epoll�¼�ע������:reactive_socket_service_base::start_accept_op->reactive_socket_service_base::start_op
//������epoll�¼�ע������:reactive_descriptor_service::async_read_some->reactive_descriptor_service::start_op->epoll_reactor::start_op
//д����epoll�¼�ע������:reactive_descriptor_service::async_write_some->reactive_descriptor_service::start_op->epoll_reactor::start_op
*/


//��ȡmongoЭ�鱨��ͷ����Э��ջ���غ����_headerCallback�ص�
//TransportLayerASIO::ASIOTicket::fill
void TransportLayerASIO::ASIOSourceTicket::fillImpl() {  //���յ�fillImpl
    auto session = getSession();
    if (!session)
        return;

    const auto initBufSize = kHeaderSize;
    _buffer = SharedBuffer::allocate(initBufSize);

	//��ȡ���� TransportLayerASIO::ASIOSession::read
    session->read(isSync(),
                  asio::buffer(_buffer.get(), initBufSize), //�ȶ�ȡͷ���ֶγ���
                  [this](const std::error_code& ec, size_t size) { _headerCallback(ec, size); });
}

void TransportLayerASIO::ASIOSinkTicket::_sinkCallback(const std::error_code& ec, size_t size) {
    networkCounter.hitPhysicalOut(_msgToSend.size()); //���͵������ֽ���ͳ��
    //sink���͵Ļص���ServiceStateMachine::_sinkCallback
    finishFill(ec ? errorCodeToStatus(ec) : Status::OK());
}

//���͵�fillImpl
void TransportLayerASIO::ASIOSinkTicket::fillImpl() {
	//��ȡ��Ӧsession
    auto session = getSession();
    if (!session)
        return;

	//�������� TransportLayerASIO::ASIOSession::write
    session->write(isSync(),
	   asio::buffer(_msgToSend.buf(), _msgToSend.size()),
	   //�������ݳɹ����callback�ص�
	   [this](const std::error_code& ec, size_t size) { _sinkCallback(ec, size); });
}

//ServiceStateMachine::_sourceMessage->TransportLayerASIO::asyncWait->TransportLayerASIO::ASIOTicket::fill->TransportLayerASIO::ASIOTicket::finishFill
//ASIOTicket::fill��_fillCallback�ص���ֵ��ASIOTicket::finishFillִ��_fillCallback�ص�
void TransportLayerASIO::ASIOTicket::finishFill(Status status) { //ÿһ���׶δ������ͨ������ִ����Ӧ�Ļص�
    // We want to make sure that a Ticket can only be filled once; filling a ticket invalidates it.
    // So we check that the _fillCallback is set, then move it out of the ticket and into a local
    // variable, and then call that. It's illegal to interact with the ticket after calling the
    // fillCallback, so we have to move it out of _fillCallback so there are no writes to any
    // variables in ASIOTicket after it gets called.
    invariant(_fillCallback);
    auto fillCallback = std::move(_fillCallback);
	//���ݽ��ն�ӦServiceStateMachine::_sourceCallback
	//���ݷ��Ͷ�ӦServiceStateMachine::_sinkCallback
    fillCallback(status); //TransportLayerASIO::asyncWait�и�ֵ��������ServiceStateMachine::_sourceCallback
}

//ServiceStateMachine::_sourceMessage->TransportLayerASIO::asyncWait->TransportLayerASIO::ASIOTicket::fill->TransportLayerASIO::ASIOTicket::finishFill
//ASIOTicket::fill��_fillCallback�ص���ֵ��ASIOTicket::finishFillִ��_fillCallback�ص�
void TransportLayerASIO::ASIOTicket::fill(bool sync, TicketCallback&& cb) {
	//ͬ�������첽��ASIO��Ӧ�첽
    _fillSync = sync;
    dassert(!_fillCallback);
	//cb��ֵ��fill�ص���cb�������ݹ��̶�ӦServiceStateMachine::_sourceCallback
	//��cb�������ݹ��̶�ӦServiceStateMachine::_sinkCallback
    _fillCallback = std::move(cb);
	//���ն�ӦTransportLayerASIO::ASIOSourceTicket::fillImpl
	//���Ͷ�ӦTransportLayerASIO::ASIOSinkTicket::fillImpl
    fillImpl();
}

}  // namespace transport
}  // namespace mongo
