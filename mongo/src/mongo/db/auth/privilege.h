/*    Copyright 2012 10gen Inc.
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
 *    must comply with the GNU Affero General Public License in all respects
 *    for all of the code used other than as permitted herein. If you modify
 *    file(s) with this exception, you may extend this exception to your
 *    version of the file(s), but you are not obligated to do so. If you do not
 *    wish to do so, delete this exception statement from your version. If you
 *    delete this exception statement from all source files in the program,
 *    then also delete it in the license file.
 */

#pragma once

#include <vector>

#include "mongo/db/auth/action_set.h"
#include "mongo/db/auth/action_type.h"
#include "mongo/db/auth/resource_pattern.h"

namespace mongo {

class Privilege;
typedef std::vector<Privilege> PrivilegeVector;

/* userInfo��mongo-cfg��ȡ�����û���Ϣ
{
	users: [{
		_id: "admin.123456",
		user: "123456",
		db: "admin",
		credentials: {
			SCRAM - SHA - 1: {
				iterationCount: 10000,
				salt: "HdWvyPNNnp43/oHayn4RUg==",
				storedKey: "a1b/EWwsMce4HVJ4V2DedhLntFg=",
				serverKey: "bV48/bWw4nSQO7qY42cGHWL09Kg="
			}
		},
		roles: [{
			role: "readWrite",
			db: "test1"
		}],
		inheritedRoles: [{
			role: "readWrite",
			db: "test1"
		}],
		inheritedPrivileges: [{
			resource: {
				db: "test1",
				collection: ""
			},
			actions: ["changeStream", "collStats", "convertToCapped", "createCollection", "createIndex", "dbHash", "dbStats", "dropCollection", "dropIndex", "emptycapped", "find", "insert", "killCursors", "listCollections", "listIndexes", "planCacheRead", "remove", "renameCollectionSameDB", "update"]
		}, {
			resource: {
				db: "test1",
				collection: "system.indexes"
			},
			actions: ["changeStream", "collStats", "dbHash", "dbStats", "find", "killCursors", "listCollections", "listIndexes", "planCacheRead"]
		}, {
			resource: {
				db: "test1",
				collection: "system.js"
			},
			actions: ["changeStream", "collStats", "convertToCapped", "createCollection", "createIndex", "dbHash", "dbStats", "dropCollection", "dropIndex", "emptycapped", "find", "insert", "killCursors", "listCollections", "listIndexes", "planCacheRead", "remove", "renameCollectionSameDB", "update"]
		}, {
			resource: {
				db: "test1",
				collection: "system.namespaces"
			},
			actions: ["changeStream", "collStats", "dbHash", "dbStats", "find", "killCursors", "listCollections", "listIndexes", "planCacheRead"]
		}],
		inheritedAuthenticationRestrictions: [],
		authenticationRestrictions: []
	}],
	ok: 1.0,
	operationTime: Timestamp(1553674933, 1),
	$replData: {
		term: 12,
		lastOpCommitted: {
			ts: Timestamp(1553674933, 1),
			t: 12
		},
		lastOpVisible: {
			ts: Timestamp(1553674933, 1),
			t: 12
		},
		configVersion: 1,
		replicaSetId: ObjectId('5c6e1c764e3e991ab8278bd9'),
		primaryIndex: 0,
		syncSourceIndex: -1
	},
	$gleStats: {
		lastOpTime: {
			ts: Timestamp(1553674933, 1),
			t: 12
		},
		electionId: ObjectId('7fffffff000000000000000c')
	},
	$clusterTime: {
		clusterTime: Timestamp(1553674933, 1),
		signature: {
			hash: BinData(0, 0000000000000000000000000000000000000000),
			keyId: 0
		}
	}
}
*/

/**
 * A representation of the permission to perform a set of actions on a resource.
 */  
/*
��Ҫ��Action��Resource��װ��Ȼ�����_isAuthorizedForPrivilege��ɹ��ܡ�

�򵥽�����Privilege��Action���Ƕ����ݵĲ���������Query��Insert�����Թ���ΪAction��Resource�������ݼ��ϣ�
������Collection��Ҳ������DB����Privilege����Action*Privilege����ϣ�һ��Privilege���Ժ��ж��Action��
����Privilegeά���ϣ�Action��ֻ����һ�������߱��ʽ��Resource��ϡ�Privilege�ļ��Ͽ�����ϳ�Role���
�����û����á�
*/
//������Բο�CreateIndexesCmd::addRequiredPrivileges
//���������ü�AuthorizationSession::_isAuthorizedForPrivilege
class Privilege {
public:
    /**
     * Adds "privilegeToAdd" to "privileges", de-duping "privilegeToAdd" if the vector already
     * contains a privilege on the same resource.
     *
     * This method is the preferred way to add privileges to  privilege vectors.
     */
    static void addPrivilegeToPrivilegeVector(PrivilegeVector* privileges,
                                              const Privilege& privilegeToAdd);

    static void addPrivilegesToPrivilegeVector(PrivilegeVector* privileges,
                                               const PrivilegeVector& privilegesToAdd);


    Privilege(){};
    Privilege(const ResourcePattern& resource, const ActionType& action);
    Privilege(const ResourcePattern& resource, const ActionSet& actions);
    ~Privilege() {}

    const ResourcePattern& getResourcePattern() const {
        return _resource;
    }

    const ActionSet& getActions() const {
        return _actions;
    }

    void addActions(const ActionSet& actionsToAdd);
    void removeActions(const ActionSet& actionsToRemove);

    // Checks if the given action is present in the Privilege.
    bool includesAction(const ActionType& action) const;
    // Checks if the given actions are present in the Privilege.
    bool includesActions(const ActionSet& actions) const;

    BSONObj toBSON() const;

private:
    /*
    ��Ҫ��Action��Resource��װ��Ȼ�����_isAuthorizedForPrivilege��ɹ��ܡ�
    
    �򵥽�����Privilege��Action���Ƕ����ݵĲ���������Query��Insert�����Թ���ΪAction��Resource�������ݼ��ϣ�
    ������Collection��Ҳ������DB����Privilege����Action*Privilege����ϣ�һ��Privilege���Ժ��ж��Action��
    ����Privilegeά���ϣ�Action��ֻ����һ�������߱��ʽ��Resource��ϡ�Privilege�ļ��Ͽ�����ϳ�Role���
    �����û����á�
    */
    //������Բο�CreateIndexesCmd::addRequiredPrivileges
    ResourcePattern _resource;
    ActionSet _actions;  // bitmask of actions this privilege grants
};

}  // namespace mongo
