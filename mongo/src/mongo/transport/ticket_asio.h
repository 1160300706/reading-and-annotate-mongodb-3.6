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

#pragma once

#include "mongo/transport/transport_layer_asio.h"

#include "asio.hpp"

namespace mongo {
namespace transport {

//TransportLayerASIO�����ؽӿ�ʹ��  
//�����ASIOSinkTicket��ASIOSourceTicket�̳и���,���ڿ������ݵķ��ͺͽ���
class TransportLayerASIO::ASIOTicket : public TicketImpl {
    MONGO_DISALLOW_COPYING(ASIOTicket);

public:
    //��ʼ������
    explicit ASIOTicket(const ASIOSessionHandle& session, Date_t expiration);
    //��ȡsessionId
    SessionId sessionId() const final {
        return _sessionId;
    }
    //asioģʽû�ã����legacyģ��
    Date_t expiration() const final {
        return _expiration;
    }

    /**
     * Run this ticket's work item.
     */
    void fill(bool sync, TicketCallback&& cb);

protected:
    void finishFill(Status status);
    std::shared_ptr<ASIOSession> getSession();
    bool isSync() const;

    // This must be implemented by the Source/Sink subclasses as the actual implementation
    // of filling the ticket.
    virtual void fillImpl() = 0;

private:
    //�Ự��Ϣ
    std::weak_ptr<ASIOSession> _session;
    //ÿ��session��һ��Ψһid
    const SessionId _sessionId;
    //asioģ��û�ã����legacy��Ч
    const Date_t _expiration;
    //һ������mongodbЭ�鱨�ķ��ͻ��߽�����ɺ�Ļص�����
    //cb��ֵ��fill�ص���cb�������ݹ��̶�ӦServiceStateMachine::_sourceCallback
	//��cb�������ݹ��̶�ӦServiceStateMachine::_sinkCallback
    TicketCallback _fillCallback;
    //ͬ����ʽ�����첽��ʽ�������ݴ���Ĭ���첽
    bool _fillSync;
};

/*
Ticket_asio.h (src\mongo\transport):class TransportLayerASIO::ASIOSourceTicket : public TransportLayerASIO::ASIOTicket {
Ticket_asio.h (src\mongo\transport):class TransportLayerASIO::ASIOSinkTicket : public TransportLayerASIO::ASIOTicket {
*/
//TransportLayerASIO�����ؽӿ�ʹ��   TransportLayerASIO::sourceMessage����ʹ��
//���ݽ��յ�ticket
class TransportLayerASIO::ASIOSourceTicket : public TransportLayerASIO::ASIOTicket {
public:
    //��ʼ������
    ASIOSourceTicket(const ASIOSessionHandle& session, Date_t expiration, Message* msg);

protected:
    void fillImpl() final;

private:
    void _headerCallback(const std::error_code& ec, size_t size);
    void _bodyCallback(const std::error_code& ec, size_t size);

    //�洢���ݵ�buffer��������ݻ�ȡ��Ϻ󣬻�ת�浽_target��
    SharedBuffer _buffer;
    //���ݸ�ֵ��TransportLayerASIO::ASIOSourceTicket::_bodyCallback
    //��ʼ�ռ丳ֵ��ServiceStateMachine::_sourceMessage->Session::sourceMessage->TransportLayerASIO::sourceMessage
    Message* _target; 
};

//TransportLayerASIO�����ؽӿ�ʹ��
//���ݷ��͵�ticket
class TransportLayerASIO::ASIOSinkTicket : public TransportLayerASIO::ASIOTicket {
public:
    //��ʼ������
    ASIOSinkTicket(const ASIOSessionHandle& session, Date_t expiration, const Message& msg);

protected:
    void fillImpl() final;

private:
    void _sinkCallback(const std::error_code& ec, size_t size);
    //��Ҫ���͵�����message��Ϣ
    Message _msgToSend;
};

}  // namespace transport
}  // namespace mongo

