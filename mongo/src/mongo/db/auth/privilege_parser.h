/**
 *    Copyright (C) 2013 10gen Inc.
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

#include <string>
#include <vector>

#include "mongo/base/string_data.h"
#include "mongo/db/jsobj.h"

namespace mongo {
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
//��ȡ����inheritedPrivileges�µĵ���item����������뵽��ParsedResource�࣬��parsedPrivilegeToPrivilege

class Privilege;

/**
 * This class is used to parse documents describing resources as they are represented as part
 * of privileges granted to roles in the role management commands.
 */
class ParsedResource {
    MONGO_DISALLOW_COPYING(ParsedResource);

public:
    //
    // schema declarations
    //

    static const BSONField<bool> anyResource;
    static const BSONField<bool> cluster;
    static const BSONField<std::string> db;
    static const BSONField<std::string> collection;

    //
    // construction / destruction
    //

    ParsedResource();
    ~ParsedResource();

    /** Copies all the fields present in 'this' to 'other'. */
    void cloneTo(ParsedResource* other) const;

    //
    // bson serializable interface implementation
    //

    bool isValid(std::string* errMsg) const;
    BSONObj toBSON() const;
    bool parseBSON(const BSONObj& source, std::string* errMsg);
    void clear();
    std::string toString() const;

    //
    // individual field accessors
    //

    void setAnyResource(bool anyResource);
    void unsetAnyResource();
    bool isAnyResourceSet() const;
    bool getAnyResource() const;

    void setCluster(bool cluster);
    void unsetCluster();
    bool isClusterSet() const;
    bool getCluster() const;

    void setDb(StringData db);
    void unsetDb();
    bool isDbSet() const;
    const std::string& getDb() const;

    void setCollection(StringData collection);
    void unsetCollection();
    bool isCollectionSet() const;
    const std::string& getCollection() const;

private:
    // Convention: (M)andatory, (O)ptional

    // (O) Only present if the resource matches anything.
    bool _anyResource;
    bool _isAnyResourceSet;

    // (O) Only present if the resource is the cluster
    bool _cluster;
    bool _isClusterSet;

    // (O) database portion of the resource
    std::string _db;
    bool _isDbSet;

    // (O) collection portion of the resource
    std::string _collection;
    bool _isCollectionSet;
};

/**
 * This class is used to parse documents describing privileges in the role managment commands.
 */
class ParsedPrivilege {
    MONGO_DISALLOW_COPYING(ParsedPrivilege);

public:
    //
    // schema declarations
    //

    static const BSONField<std::vector<std::string>> actions;
    static const BSONField<ParsedResource> resource;

    //
    // construction / destruction
    //

    ParsedPrivilege();
    ~ParsedPrivilege();

    /**
     * Takes a parsedPrivilege and turns it into a true Privilege object.
     * If the parsedPrivilege contains any unrecognized privileges it will add those to
     * unrecognizedActions.
     */
    static Status parsedPrivilegeToPrivilege(const ParsedPrivilege& parsedPrivilege,
                                             Privilege* result,
                                             std::vector<std::string>* unrecognizedActions);
    /**
     * Takes a Privilege object and turns it into a ParsedPrivilege.
     */
    static bool privilegeToParsedPrivilege(const Privilege& privilege,
                                           ParsedPrivilege* result,
                                           std::string* errmsg);

    //
    // bson serializable interface implementation
    //

    bool isValid(std::string* errMsg) const;
    BSONObj toBSON() const;
    bool parseBSON(const BSONObj& source, std::string* errMsg);
    void clear();
    std::string toString() const;

    //
    // individual field accessors
    //

    void setActions(const std::vector<std::string>& actions);
    void addToActions(const std::string& actions);
    void unsetActions();
    bool isActionsSet() const;
    size_t sizeActions() const;
    const std::vector<std::string>& getActions() const;
    const std::string& getActionsAt(size_t pos) const;

    void setResource(const ParsedResource& resource);
    void unsetResource();
    bool isResourceSet() const;
    const ParsedResource& getResource() const;
    void setIsCommonUserRole(bool role);
    const bool getIsCommonUserRole() const;

private:
    // Convention: (M)andatory, (O)ptional

    // (M) Array of action types
    std::vector<std::string> _actions;
    bool _isActionsSet;

    // (M) Object describing the resource pattern of this privilege
    ParsedResource _resource;
    bool _isResourceSet;

    bool isCommonUserRole;
};

}  // namespace mongo
