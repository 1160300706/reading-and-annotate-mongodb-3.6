/**
 *    Copyright (C) 2014 MongoDB Inc.
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

#include <cstdint>
#include <limits>
#include <string>

#include "mongo/base/static_assert.h"
#include "mongo/base/string_data.h"
#include "mongo/config.h"
#include "mongo/platform/hash_namespace.h"

namespace mongo {

class Locker;

struct LockHead;
struct PartitionedLockHead;

/*
mode_x���
[root@bogon mongo]# grep "MODE_X" * -r | grep -v "test" |grep -v "mmap" | grep -v "//" | grep -v ".h" |grep -v "*" | grep -v "invariant" |grep -v "dassert" |grep -v "lock_manager" |grep -v "lock_state"
db/catalog/capped_utils.cpp:    AutoGetDb autoDb(opCtx, collectionName.db(), MODE_X);
db/catalog/capped_utils.cpp:    AutoGetDb autoDb(opCtx, collectionName.db(), MODE_X);
db/catalog/coll_mod.cpp:    AutoGetDb autoDb(opCtx, dbName, MODE_X);
db/catalog/coll_mod.cpp:        AutoGetDb autoDb(opCtx, dbname, MODE_X);
db/catalog/coll_mod.cpp:        AutoGetDb autoDb(opCtx, dbname, MODE_X);
db/catalog/coll_mod.cpp:        AutoGetDb autoDb(opCtx, dbname, MODE_X);
db/catalog/coll_mod.cpp:        AutoGetDb autoDb(opCtx, dbname, MODE_X);
db/catalog/collection_impl.cpp:            opCtx->lockState(), ResourceId(RESOURCE_METADATA, _ns.ns()), MODE_X};
db/catalog/collection_impl.cpp:            opCtx->lockState(), ResourceId(RESOURCE_METADATA, _ns.ns()), MODE_X};
db/catalog/create_collection.cpp:        Lock::DBLock dbXLock(opCtx, nss.db(), MODE_X);
db/catalog/drop_collection.cpp:        AutoGetDb autoDb(opCtx, dbname, MODE_X);
db/catalog/drop_database.cpp:        AutoGetDb autoDB(opCtx, dbName, MODE_X);
db/catalog/drop_database.cpp:        AutoGetDb autoDB(opCtx, dbName, MODE_X);
db/catalog/drop_database.cpp:        AutoGetDb autoDB(opCtx, dbName, MODE_X);
db/catalog/drop_indexes.cpp:            AutoGetDb autoDb(opCtx, nss.db(), MODE_X);
db/catalog/index_catalog_entry_impl.cpp:            opCtx->lockState(), ResourceId(RESOURCE_METADATA, _ns), MODE_X);
db/catalog/rename_collection.cpp:        dbWriteLock.emplace(opCtx, source.db(), MODE_X);
db/cloner.cpp:    Lock::DBLock dbWrite(opCtx, dbname, MODE_X);
db/commands/clone.cpp:        Lock::DBLock dbXLock(opCtx, dbname, MODE_X);
db/commands/collection_to_capped.cpp:        AutoGetDb autoDb(opCtx, dbname, MODE_X);
db/commands/compact.cpp:        AutoGetDb autoDb(opCtx, db, MODE_X);
db/commands/copydb.cpp:            Lock::DBLock lk(opCtx, todb, MODE_X);
db/commands/cpuprofile.cpp:    Lock::DBLock dbXLock(opCtx, db, MODE_X);
db/commands/cpuprofile.cpp:    Lock::DBLock dbXLock(opCtx, db, MODE_X);
db/commands/create_indexes.cpp:        Lock::DBLock dbLock(opCtx, ns.db(), MODE_X);
db/commands/dbcommands.cpp:        const LockMode dbMode = readOnly ? MODE_S : MODE_X;
db/commands/drop_indexes.cpp:        Lock::DBLock dbXLock(opCtx, dbname, MODE_X);
db/commands/mr.cpp:            AutoGetDb autoDb(_opCtx, _config.tempNamespace.db(), MODE_X);
db/commands/mr.cpp:            Lock::DBLock lk(_opCtx, _config.incLong.db(), MODE_X);
db/commands/mr.cpp:            Lock::DBLock lock(opCtx, _config.outputOptions.finalNamespace.db(), MODE_X);
db/commands/validate.cpp:        auto collLk = stdx::make_unique<Lock::CollectionLock>(opCtx->lockState(), nss.ns(), MODE_X);
db/concurrency/d_concurrency.cpp:    return locker->isLockHeldForMode(_rid, MODE_X);
db/concurrency/d_concurrency.cpp:        _mode = MODE_X;
db/db.cpp:    AutoGetOrCreateDb autoDb(opCtx, startupLogCollectionName.db(), mongo::MODE_X);
db/db.cpp:        Lock::DBLock dbLock(opCtx, kSystemReplSetCollection.db(), MODE_X);
db/db.cpp:    LockResult result = globalLocker->lockGlobalBegin(MODE_X, Milliseconds::max());
db/db_raii.cpp:        if (mode != MODE_X) {
db/index_builder.cpp:    Lock::DBLock dlk(&opCtx, ns.db(), MODE_X);
db/index_rebuilder.cpp:        Lock::DBLock lk(opCtx, nss.db(), MODE_X);
db/introspect.cpp:                autoGetDb.reset(new AutoGetDb(opCtx, dbName, MODE_X));
db/introspect.cpp:                       (!wasLocked || opCtx->lockState()->isDbLockedForMode(dbName, MODE_X))) {
db/ops/update.cpp:                  locker->isLockHeldForMode(ResourceId(RESOURCE_DATABASE, nsString.db()), MODE_X));
db/ops/update.cpp:            Lock::DBLock lk(opCtx, nsString.db(), MODE_X);
db/ops/write_ops_exec.cpp:        AutoGetOrCreateDb db(opCtx, ns.db(), MODE_X);
db/ops/write_ops_exec.cpp:                           parsedUpdate.isIsolated() ? MODE_X : MODE_IX);
db/ops/write_ops_exec.cpp:                                 parsedDelete.isIsolated() ? MODE_X : MODE_IX);
db/repl/bgsync.cpp:                Lock::DBLock lk(opCtx, "local", MODE_X);
db/repl/master_slave.cpp:        Lock::DBLock dblk(opCtx, "local", MODE_X);
db/repl/oplog.cpp:            requestNss.ns(), supportsDocLocking() ? MODE_IX : MODE_X));
db/repl/replication_coordinator_external_state_impl.cpp:        Lock::DBLock lock(opCtx, meDatabaseName, MODE_X);
db/repl/replication_coordinator_external_state_impl.cpp:            Lock::DBLock dbWriteLock(opCtx, configDatabaseName, MODE_X);
db/repl/replication_coordinator_external_state_impl.cpp:                Lock::DBLock dbWriteLock(opCtx, lastVoteDatabaseName, MODE_X);
db/repl/replication_coordinator_impl.cpp:        opCtx, MODE_X, durationCount<Milliseconds>(stepdownTime), Lock::GlobalLock::EnqueueOnly());
db/repl/replication_coordinator_impl.cpp:                globalLock.reset(new Lock::GlobalLock(opCtx, MODE_X, UINT_MAX));
db/repl/replication_recovery.cpp:    Lock::CollectionLock oplogCollectionLoc(opCtx->lockState(), oplogNss.ns(), MODE_X);
db/repl/rs_rollback.cpp:    Lock::DBLock dbLock(opCtx, nss.db(), MODE_X);
db/repl/rs_rollback.cpp:    Lock::DBLock dbLock(opCtx, nss.db(), MODE_X);
db/repl/rs_rollback.cpp:    Lock::DBLock dbLock(opCtx, dbName, MODE_X);
db/repl/rs_rollback.cpp:        AutoGetDb dbLock(opCtx, nss.db(), MODE_X);
db/repl/rs_rollback.cpp:            Lock::DBLock dbLock(opCtx, nss.db(), MODE_X);
db/repl/rs_rollback.cpp:                Lock::DBLock docDbLock(opCtx, docNss.db(), MODE_X);
db/repl/rs_rollback.cpp:        Lock::CollectionLock oplogCollectionLoc(opCtx->lockState(), oplogNss.ns(), MODE_X);
db/repl/rs_rollback_no_uuid.cpp:                Lock::DBLock dbLock(opCtx, nss.db(), MODE_X);
db/repl/rs_rollback_no_uuid.cpp:            Lock::DBLock dbLock(opCtx, nss.db(), MODE_X);
db/repl/rs_rollback_no_uuid.cpp:        Lock::DBLock dbLock(opCtx, nss.db(), MODE_X);
db/repl/rs_rollback_no_uuid.cpp:        Lock::DBLock dbLock(opCtx, nss.db(), MODE_X);
db/repl/rs_rollback_no_uuid.cpp:                Lock::DBLock docDbLock(opCtx, docNss.db(), MODE_X);
db/repl/rs_rollback_no_uuid.cpp:        Lock::CollectionLock oplogCollectionLoc(opCtx->lockState(), oplogNss.ns(), MODE_X);
db/repl/storage_interface_impl.cpp:        AutoGetOrCreateDb db(opCtx.get(), nss.db(), MODE_X);
db/repl/storage_interface_impl.cpp:        AutoGetOrCreateDb databaseWriteGuard(opCtx, nss.db(), MODE_X);
db/repl/storage_interface_impl.cpp:        AutoGetDb autoDB(opCtx, nss.db(), MODE_X);
db/repl/storage_interface_impl.cpp:        AutoGetCollection autoColl(opCtx, nss, MODE_X);
db/repl/storage_interface_impl.cpp:        AutoGetDb autoDB(opCtx, fromNS.db(), MODE_X);
db/repl/storage_interface_impl.cpp:    AutoGetDb autoDB(opCtx, "admin", MODE_X);
db/repl/sync_tail.cpp:        Lock::DBLock dbLock(opCtx, nss.db(), MODE_X);
db/repl/sync_tail.cpp:        AutoGetDb autoDb(opCtx, nss.db(), MODE_X);
db/s/migration_destination_manager.cpp:        Lock::DBLock lk(opCtx, _nss.db(), MODE_X);
db/s/migration_destination_manager.cpp:            Lock::CollectionLock clk(opCtx->lockState(), nss.ns(), MODE_X);
db/s/migration_destination_manager.cpp:    AutoGetCollection autoColl(opCtx, nss, MODE_IX, MODE_X);
db/s/migration_destination_manager.cpp:    AutoGetCollection autoColl(opCtx, nss, MODE_IX, MODE_X);
db/s/migration_source_manager.cpp:        AutoGetCollection autoColl(opCtx, getNss(), MODE_IX, MODE_X);
db/s/migration_source_manager.cpp:        AutoGetCollection autoColl(opCtx, getNss(), MODE_IX, MODE_X);
db/s/migration_source_manager.cpp:        AutoGetCollection autoColl(opCtx, getNss(), MODE_IX, MODE_X);
db/s/migration_source_manager.cpp:        AutoGetCollection autoColl(opCtx, getNss(), MODE_IX, MODE_X);
db/s/migration_source_manager.cpp:        AutoGetCollection autoColl(opCtx, getNss(), MODE_IX, MODE_X);
db/storage/wiredtiger/wiredtiger_record_store.cpp:            !opCtx->lockState()->isCollectionLockedForMode(_ns, MODE_X)) {
db/storage/wiredtiger/wiredtiger_record_store.cpp:            !opCtx->lockState()->isCollectionLockedForMode(_ns, MODE_X)) {
db/system_index.cpp:        AutoGetDb autoDb(opCtx, systemUsers.db(), MODE_X);



mode_s���
[root@bogon mongo]# grep "MODE_S" * -r | grep -v "test" |grep -v "mmap" | grep -v "//" | grep -v ".h" |grep -v "*" | grep -v "invariant" |grep -v "dassert" |grep -v "lock_manager" |grep -v "lock_state"
db/commands/dbcommands.cpp:        const LockMode dbMode = readOnly ? MODE_S : MODE_X;
db/commands/dbcommands.cpp:        AutoGetDb autoDb(opCtx, ns, MODE_S);
db/commands/list_collections.cpp:        AutoGetDb autoDb(opCtx, dbname, MODE_S);
db/commands/mr.cpp:                unique_ptr<AutoGetDb> scopedAutoDb(new AutoGetDb(opCtx, config.nss.db(), MODE_S));
db/commands/mr.cpp:                        scopedAutoDb = stdx::make_unique<AutoGetDb>(opCtx, config.nss.db(), MODE_S);
db/commands/mr.cpp:                        scopedAutoDb.reset(new AutoGetDb(opCtx, config.nss.db(), MODE_S));
db/repl/oplog_interface_local.cpp:      _collectionLock(opCtx->lockState(), collectionName, MODE_S),
util/net/ssl_manager.cpp:#if defined(MONGO_CONFIG_HAVE_FIPS_MODE_SET)

MODE_Is���
[root@bogon mongo]# grep "MODE_IS" * -r | grep -v "test" |grep -v "mmap" | grep -v "//" | grep -v ".h" |grep -v "*" | grep -v "invariant" |grep -v "dassert" |grep -v "lock_manager" |grep -v "lock_state"
db/catalog/coll_mod.cpp:        Lock::GlobalLock lk(opCtx, MODE_IS, UINT_MAX);
db/catalog/coll_mod.cpp:        Lock::GlobalLock lk(opCtx, MODE_IS, UINT_MAX);
db/clientcursor.cpp:        _opCtx->lockState()->isCollectionLockedForMode(_cursor->_nss.ns(), MODE_IS);
db/clientcursor.cpp:        _opCtx->lockState()->isCollectionLockedForMode(_cursor->_nss.ns(), MODE_IS);
db/commands/count_cmd.cpp:        Lock::DBLock dbLock(opCtx, dbname, MODE_IS);
db/commands/count_cmd.cpp:        Lock::DBLock dbLock(opCtx, dbname, MODE_IS);
db/commands/find_cmd.cpp:        Lock::DBLock dbSLock(opCtx, dbname, MODE_IS);
db/commands/fsync.cpp:            Lock::GlobalLock global(opCtx, MODE_IS, UINT_MAX);
db/commands/list_databases.cpp:            Lock::GlobalLock lk(opCtx, MODE_IS, UINT_MAX);
db/commands/list_databases.cpp:                Lock::DBLock dbLock(opCtx, dbname, MODE_IS);
db/commands/list_indexes.cpp:        Lock::DBLock dbSLock(opCtx, dbname, MODE_IS);
db/commands/mr.cpp:            AutoGetCollection autoColl(opCtx, _config.incLong, MODE_IS);
db/commands/parallel_collection_scan.cpp:        Lock::DBLock dbSLock(opCtx, dbname, MODE_IS);
db/commands/run_aggregate.cpp:    AutoGetDb autoDb(opCtx, request.getNamespaceString().db(), MODE_IS);
db/commands/run_aggregate.cpp:            AutoGetCollection origNssCtx(opCtx, origNss, MODE_IS);
db/concurrency/d_concurrency.cpp:    return locker->isLockHeldForMode(_rid, MODE_IS);
db/concurrency/d_concurrency.cpp:        _pbwm.lock(MODE_IS);
db/cursor_manager.cpp:        AutoGetCollectionOrView ctx(opCtx, NamespaceString(ns), MODE_IS);
db/db_raii.cpp:        AutoGetDb autoDb(_opCtx, nss.db(), MODE_IS);
db/db_raii.cpp:    Lock::DBLock dbSLock(opCtx, dbName, MODE_IS);
db/db_raii.cpp:            opCtx, nss, MODE_IS, AutoGetCollection::ViewMode::kViewsForbidden, std::move(dbSLock));
db/db_raii.cpp:    _autoColl.emplace(opCtx, nss, MODE_IS, MODE_IS, viewMode);
db/db_raii.cpp:    _autoColl.emplace(opCtx, nss, MODE_IS, viewMode, std::move(lock));
db/db_raii.cpp:        _autoColl.emplace(opCtx, nss, MODE_IS);
db/db_raii.cpp:          opCtx, nss, viewMode, Lock::DBLock(opCtx, nss.db(), MODE_IS)) {}
db/pipeline/document_source_cursor.cpp:    AutoGetDb dbLock(opCtx, _exec->nss().db(), MODE_IS);
db/pipeline/document_source_cursor.cpp:    Lock::CollectionLock collLock(opCtx->lockState(), _exec->nss().ns(), MODE_IS);
db/pipeline/pipeline_d.cpp:            AutoGetCollection autoColl(_ctx->opCtx, _ctx->ns, MODE_IS);
db/pipeline/pipeline_d.cpp:            AutoGetCollection autoColl(_ctx->opCtx, nss, collectionUUID, MODE_IS);
db/query/find.cpp:            AutoGetDb autoDb(opCtx, nssForCurOp->db(), MODE_IS);
db/repl/oplog_interface_local.cpp:    : _dbLock(opCtx, nsToDatabase(collectionName), MODE_IS),
db/repl/replication_coordinator_external_state_impl.cpp:                               AutoGetCollection oplog(opCtx, kRsOplogNamespace, MODE_IS);
db/repl/replication_coordinator_external_state_impl.cpp:    AutoGetCollection oplog(opCtx, NamespaceString::kRsOplogNamespace, MODE_IS);
db/repl/storage_interface_impl.cpp:        auto collectionAccessMode = isFind ? MODE_IS : MODE_IX;
db/repl/storage_interface_impl.cpp:    AutoGetCollection oplog(opCtx, NamespaceString::kRsOplogNamespace, MODE_IS);
db/repl/sync_tail.cpp:        Lock::DBLock dbLock(opCtx, nsToDatabaseSubstring(ns), MODE_IS);
db/s/active_migrations_registry.cpp:        AutoGetCollection autoColl(opCtx, nss.get(), MODE_IS);
db/s/migration_source_manager.cpp:        AutoGetCollection autoColl(opCtx, getNss(), MODE_IS);
db/s/migration_source_manager.cpp:            AutoGetCollection autoColl(opCtx, _args.getNss(), MODE_IS);
db/s/migration_source_manager.cpp:            AutoGetCollection autoColl(opCtx, _args.getNss(), MODE_IS);
db/s/migration_source_manager.cpp:        AutoGetCollection autoColl(opCtx, getNss(), MODE_IS);
db/s/migration_source_manager.cpp:        AutoGetCollection autoColl(opCtx, getNss(), MODE_IS);
db/s/split_vector.cpp:        AutoGetCollection autoColl(opCtx, nss, MODE_IS);
db/session_catalog.cpp:    AutoGetCollection autoColl(opCtx, NamespaceString::kSessionTransactionsTableNamespace, MODE_IS);
db/sessions_collection_rs.cpp:                    MODE_IS,
db/storage/wiredtiger/wiredtiger_server_status.cpp:    Lock::GlobalLock lk(opCtx, LockMode::MODE_IS, UINT_MAX);
db/transaction_reaper.cpp:        Lock::DBLock lk(opCtx, NamespaceString::kSessionTransactionsTableNamespace.db(), MODE_IS);
db/transaction_reaper.cpp:            opCtx->lockState(), NamespaceString::kSessionTransactionsTableNamespace.ns(), MODE_IS);
db/ttl.cpp:            AutoGetCollection autoGetCollection(&opCtx, collectionNSS, MODE_IS);
db/views/durable_view_catalog.cpp:    Lock::CollectionLock lk(opCtx->lockState(), _db->getSystemViewsName(), MODE_IS);

mode_ix���
[root@bogon mongo]# grep "MODE_IX" * -r | grep -v "test" |grep -v "mmap" | grep -v "//" | grep -v ".h" |grep -v "*" | grep -v "invariant" |grep -v "dassert" |grep -v "lock_manager" |grep -v "lock_state"
db/catalog/rename_collection.cpp:        AutoGetCollection autoTmpColl(opCtx, tmpName, MODE_IX);
db/catalog/rename_collection.cpp:            lockState->downgrade(globalLockResourceId, MODE_IX);
db/commands/create_indexes.cpp:            Lock::CollectionLock colLock(opCtx->lockState(), ns.ns(), MODE_IX);
db/commands/find_and_modify.cpp:            AutoGetCollection autoColl(opCtx, nsString, MODE_IX);
db/commands/find_and_modify.cpp:            AutoGetCollection autoColl(opCtx, nsString, MODE_IX);
db/commands/find_and_modify.cpp:                AutoGetOrCreateDb autoDb(opCtx, dbName, MODE_IX);
db/commands/find_and_modify.cpp:                Lock::CollectionLock collLock(opCtx->lockState(), nsString.ns(), MODE_IX);
db/commands/find_and_modify.cpp:                AutoGetOrCreateDb autoDb(opCtx, dbName, MODE_IX);
db/commands/find_and_modify.cpp:                Lock::CollectionLock collLock(opCtx->lockState(), nsString.ns(), MODE_IX);
db/commands/oplog_note.cpp:    Lock::GlobalLock lock(opCtx, MODE_IX, 1);
db/commands/validate.cpp:        AutoGetDb ctx(opCtx, nss.db(), MODE_IX);
db/concurrency/d_concurrency.cpp:    _lockState->lock(_id, MODE_IX);
db/concurrency/d_concurrency.cpp:    _lockState->lock(resourceIdOplog, MODE_IX);
db/concurrency/deferred_writer.cpp:    agc = stdx::make_unique<AutoGetCollection>(opCtx, _nss, MODE_IX);
db/concurrency/deferred_writer.cpp:        agc = stdx::make_unique<AutoGetCollection>(opCtx, _nss, MODE_IX);
db/db_raii.cpp:      _autodb(opCtx, _nss.db(), MODE_IX),
db/db_raii.cpp:      _collk(opCtx->lockState(), ns, MODE_IX),
db/exec/stagedebug_cmd.cpp:        AutoGetCollection autoColl(opCtx, nss, MODE_IX);
db/index_builder.cpp:                    Lock::CollectionLock colLock(opCtx->lockState(), ns.ns(), MODE_IX);
db/introspect.cpp:                autoGetDb.reset(new AutoGetDb(opCtx, dbName, MODE_IX));
db/introspect.cpp:            Lock::CollectionLock collLock(opCtx->lockState(), db->getProfilingNS(), MODE_IX);
db/ops/write_ops_exec.cpp:                           parsedUpdate.isIsolated() ? MODE_X : MODE_IX);
db/ops/write_ops_exec.cpp:                                 parsedDelete.isIsolated() ? MODE_X : MODE_IX);
db/read_concern.cpp:        Lock::DBLock lk(opCtx, "local", MODE_IX);
db/read_concern.cpp:        Lock::CollectionLock lock(opCtx->lockState(), "local.oplog.rs", MODE_IX);
db/repl/apply_ops.cpp:                        AutoGetCollection autoColl(opCtx, nss, MODE_IX);
db/repl/apply_ops.cpp:        dbWriteLock.emplace(opCtx, dbName, MODE_IX);
db/repl/mock_repl_coord_server_fixture.cpp:    AutoGetCollection autoColl(opCtx(), NamespaceString::kRsOplogNamespace, MODE_IX);
db/repl/noop_writer.cpp:    Lock::GlobalLock lock(opCtx, MODE_IX, 1);
db/repl/oplog.cpp:    Lock::DBLock lk(opCtx, NamespaceString::kLocalDb, MODE_IX);
db/repl/oplog.cpp:    Lock::CollectionLock lock(opCtx->lockState(), _oplogCollectionName, MODE_IX);
db/repl/oplog.cpp:    Lock::DBLock lk(opCtx, "local", MODE_IX);
db/repl/oplog.cpp:    Lock::CollectionLock lock(opCtx->lockState(), _oplogCollectionName, MODE_IX);
db/repl/oplog.cpp:            requestNss.ns(), supportsDocLocking() ? MODE_IX : MODE_X));
db/repl/oplog.cpp:        AutoGetCollection autoColl(opCtx, NamespaceString(_oplogCollectionName), MODE_IX);
db/repl/replication_recovery.cpp:    AutoGetDb autoDb(opCtx, oplogNss.db(), MODE_IX);
db/repl/rs_rollback.cpp:        Lock::DBLock oplogDbLock(opCtx, oplogNss.db(), MODE_IX);
db/repl/rs_rollback_no_uuid.cpp:        Lock::DBLock oplogDbLock(opCtx, oplogNss.db(), MODE_IX);
db/repl/storage_interface_impl.cpp:        AutoGetCollection coll(opCtx.get(), nss, MODE_IX);
db/repl/storage_interface_impl.cpp:        autoColl = stdx::make_unique<AutoGetCollection>(opCtx.get(), nss, MODE_IX);
db/repl/storage_interface_impl.cpp:    AutoGetCollection autoColl(opCtx, nss, MODE_IX);
db/repl/storage_interface_impl.cpp:        auto collectionAccessMode = isFind ? MODE_IS : MODE_IX;
db/repl/storage_interface_impl.cpp:        AutoGetCollection autoColl(opCtx, nss, MODE_IX);
db/repl/storage_interface_impl.cpp:        AutoGetCollection autoColl(opCtx, nss, MODE_IX);
db/repl/storage_interface_impl.cpp:        AutoGetCollection autoColl(opCtx, nss, MODE_IX);
db/repl/sync_tail.cpp:            Lock::DBLock dbLock(opCtx, nss.db(), MODE_IX);
db/repl/sync_tail.cpp:            Lock::CollectionLock collLock(opCtx->lockState(), actualNss.ns(), MODE_IX);
db/s/collection_range_deleter.cpp:        AutoGetCollection autoColl(opCtx, nss, MODE_IX);
db/s/collection_range_deleter.cpp:                    opCtx, NamespaceString::kServerConfigurationNamespace, MODE_IX);
db/s/collection_range_deleter.cpp:        AutoGetCollection autoColl(opCtx, nss, MODE_IX);
db/s/migration_destination_manager.cpp:        Lock::DBLock dlk(opCtx, nss.db(), MODE_IX);
db/s/migration_destination_manager.cpp:    AutoGetCollection autoColl(opCtx, nss, MODE_IX, MODE_X);
db/s/migration_destination_manager.cpp:    AutoGetCollection autoColl(opCtx, nss, MODE_IX, MODE_X);
db/s/migration_source_manager.cpp:        AutoGetCollection autoColl(opCtx, getNss(), MODE_IX, MODE_X);
db/s/migration_source_manager.cpp:        AutoGetCollection autoColl(opCtx, getNss(), MODE_IX, MODE_X);
db/s/migration_source_manager.cpp:        AutoGetCollection autoColl(opCtx, getNss(), MODE_IX, MODE_X);
db/s/migration_source_manager.cpp:        AutoGetCollection autoColl(opCtx, getNss(), MODE_IX, MODE_X);
db/s/migration_source_manager.cpp:    AutoGetCollection autoColl(opCtx, NamespaceString::kRsOplogNamespace, MODE_IX);
db/s/migration_source_manager.cpp:        AutoGetCollection autoColl(opCtx, getNss(), MODE_IX, MODE_X);
db/s/session_catalog_migration_destination.cpp:                opCtx, NamespaceString::kSessionTransactionsTableNamespace.db(), MODE_IX);
db/s/session_catalog_migration_source.cpp:        AutoGetCollection autoColl(opCtx, NamespaceString::kRsOplogNamespace, MODE_IX);
db/session.cpp:    AutoGetCollection autoColl(opCtx, NamespaceString::kSessionTransactionsTableNamespace, MODE_IX);
db/session.cpp:            Lock::DBLock configDBLock(opCtx, NamespaceString::kConfigDb, MODE_IX);
db/sessions_collection_rs.cpp:                    MODE_IX,
db/sessions_collection_rs.cpp:        MODE_IX,
db/sessions_collection_rs.cpp:                    MODE_IX,
db/sessions_collection_rs.cpp:        MODE_IX,
db/storage/wiredtiger/wiredtiger_record_store_mongod.cpp:            AutoGetDb autoDb(&opCtx, _ns.db(), MODE_IX);
db/storage/wiredtiger/wiredtiger_record_store_mongod.cpp:            Lock::CollectionLock collectionLock(opCtx.lockState(), _ns.ns(), MODE_IX);
db/ttl.cpp:        AutoGetCollection autoGetCollection(opCtx, collectionNSS, MODE_IX);
db/views/durable_view_catalog.cpp:            opCtx->lockState()->isDbLockedForMode(_db->name(), MODE_IX));

*/

/*
https://docs.mongodb.com/manual/faq/concurrency/

Lock Mode	Description
R	Represents Shared (S) lock.
W	Represents Exclusive (X) lock.
r	Represents Intent Shared (IS) lock.
w	Represents Intent Exclusive (IX) lock.


What locks are taken by some common client operations??
The following table lists some operations and the types of locks they use for document level locking storage engines:

Operation	                            Database	Collection
Issue a query	                        r (Intent Shared)	r (Intent Shared)
Insert data	                            w (Intent Exclusive)	w (Intent Exclusive)
Remove data	                            w (Intent Exclusive)	w (Intent Exclusive)
Update data	                            w (Intent Exclusive)	w (Intent Exclusive)
Perform Aggregation	r (Intent Shared)	r (Intent Shared)
Create an index (Foreground)	        W (Exclusive)	 
Create an index (Background)	        w (Intent Exclusive)	w (Intent Exclusive)
List collections	                    r (Intent Shared)
                                        Changed in version 4.0.
Map-reduce	                            W (Exclusive) and R (Shared)	w (Intent Exclusive) and r (Intent Shared)




MongoDB ����ʱ��������ģʽ��MODE_IS��MODE_IX��MODE_S��MODE_X����MODE_S�� MODE_X ��������⣬�ֱ��ǻ��������
����д����MODE_IS��MODE_IX��Ϊ��ʵ�ֲ����ģ������ģ���Ϊ�������������д������֮��ľ����������ͼ��ʾ��

MongoDB�ڼ���ʱ����һ������ԵĹ���ʽ���� globalLock ==> DBLock ==> CollecitonLock �� ���������Ƕ�֪��
MongoDB wiredtiger���ĵ�����������ô��д����ʱ����������������

д����

1. globalLock  (��һ��ֻ��ע�Ƕ�����д������ע������ʲôLOCK)
2. DBLock MODE_IX
3. Colleciotn MODE_IX
4. pass request to wiredtiger

������
1. globalLock MODE_IS  (��һ��ֻ��ע�Ƕ�����д������ע������ʲôLOCK)
2. DBLock MODE_IS
3. Colleciton MODE_IS
4. pass request to wiredtiger
������ͼ�ľ��������IS��IX�����辺���ģ����Զ�д���������û�о���������£�ͬʱ����wiredtiger����ȥ����

�پٸ����ӣ����һ��ǰ̨�������Ĳ�����һ�������󲢷���

ǰ̨����������

1. globalLock MODE_IX (��һ��ֻ��ע�Ƕ�����д������ע������ʲôLOCK)
2. DBLock MODE_X
3. pass to wiredtiger

������
1. globalLock MODE_IS (��һ��ֻ��ע�Ƕ�����д������ע������ʲôLOCK)
2. DBLock MODE_IS
3. Colleciton MODE_IS
4. pass request to wiredtiger
���ݾ�����MODE_X��MODE_IS��Ҫ�����ģ���Ҳ����Ϊʲôǰ̨�������Ĺ����ж��Ǳ������ġ�

���ǽ�����ܵ� globalLock ��Ӧ�����ĵ�һ������globalLock��һ�㣬ֻ�����Ƕ���������д�����������ǻ�����������������
���� globalLock ��һ���ǲ����ھ����ġ�
http://www.mongoing.com/archives/4768

���е�������ƽ�ȵģ�����������һ�����������FIFOԭ�򡣵��ǣ�MongoDB�����Ż�������һ����������ʱ��
�����������ݵ������ᱻ���ɣ��Ӷ����Բ����������ٸ����ӣ��������Collection A��
��Document aʹ��S��ʱ������reader����ͬʱʹ��S������ȡ��Document a��Ҳ����ͬʱ��ȡͬһ��Collection
��Document b.��Ϊ���е�S�����Ǽ��ݵġ���ô�������ʱ���Collection A�е�Document c����д�����Ƿ��
���أ���Ȼ��ҪΪDocument c����x������ʱCollection A����ҪIX����������IX��IS�Ǽ��ݵģ�����û�����⡣
����˵��ֻҪ����ͬһ��Document����д�����ǿ��Բ����ģ������ͬһ��Document�������Բ�������д�����ԡ�
https://www.jianshu.com/p/d838a5905303
*/ //����˵��ֻҪ����ͬһ��Document����д�����ǿ��Բ����ģ������ͬһ��Document�������Բ�������д�����ԡ�
/**
 * Lock modes.
 *
 * Compatibility Matrix  �����Թ�ϵ +���ݹ���        +�Ǽ��ݵ�   
 *                                          Granted mode
 *   ---------------.--------------------------------------------------------.
 *   Requested Mode | MODE_NONE  MODE_IS   MODE_IX  MODE_S   MODE_X  |
 *     MODE_IS      |      +        +         +        +        -    |
 *     MODE_IX      |      +        +         +        -        -    |
 *     MODE_S       |      +        +         -        +        -    |
 *     MODE_X       |      +        -         -        -        -    |  ����MODE_X���󣬶�д��������
 * �ٷ��ĵ�https://docs.mongodb.com/manual/faq/concurrency/
 */ //����ģʽ������������ticketHoldersָ������

//���²ο� https://yq.aliyun.com/articles/655101 ǳ��MongoDB�е�������
//https://mongoing.com/archives/4768
//https://mp.weixin.qq.com/s/aD6AySeHX8uqMlg9NgvppA?spm=a2c4e.11153940.blogcont655101.6.6fca281cYe2TH0
//ResourceId��(����ȫ���� ���� ����)��ÿ��ResourceId������ϸ��Ϊ��ͬ���͵�MODE_IS MODE_IX MODE_S MODE_X��

//ÿ��ģʽ��Ӧһ��TicketHolder���ο�ticketHolders
enum LockMode { //��ͬ����ͳ����LockStats��ʵ��
    MODE_NONE = 0,
    MODE_IS = 1,
    MODE_IX = 2,
    MODE_S = 3,
    MODE_X = 4,

    //static const char* LockModeNames[] = {"NONE", "IS", "IX", "S", "X"};
    //static const char* LegacyLockModeNames[] = {"", "r", "w", "R", "W"};
    
    // Counts the lock modes. Used for array size allocations, etc. Always insert new lock
    // modes above this entry.
    LockModesCount 
};

/**
 * Returns a human-readable name for the specified lock mode.
 */
const char* modeName(LockMode mode);

/**
 * Legacy lock mode names in parity for 2.6 reports.
 */
const char* legacyModeName(LockMode mode);

/**
 * Mode A is covered by mode B if the set of conflicts for mode A is a subset of the set of
 * conflicts for mode B. For example S is covered by X. IS is covered by S. However, IX is not
 * covered by S or IS.
 */
bool isModeCovered(LockMode mode, LockMode coveringMode);

/**
 * Returns whether the passed in mode is S or IS. Used for validation checks.
 */ 
//�ж��Ƿ�������߶�������
inline bool isSharedLockMode(LockMode mode) {
    return (mode == MODE_IS || mode == MODE_S);
}


/**
 * Return values for the locking functions of the lock manager.
 */
enum LockResult {

    /**
     * The lock request was granted and is now on the granted list for the specified resource.
     * �������ѱ����裬����λ��ָ����Դ�������б��С�
     */
    LOCK_OK,

    /**
     * The lock request was not granted because of conflict. If this value is returned, the
     * request was placed on the conflict queue of the specified resource and a call to the
     * LockGrantNotification::notify callback should be expected with the resource whose lock
     * was requested.
     * ���ڳ�ͻ��������δ�����衣������ش�ֵ�������������ָ����Դ�ĳ�ͻ�����У�����Ӧ������������
     * ������Դ����LockGrantNotification::notify�ص�������
     */
    LOCK_WAITING,

    /**
     * The lock request waited, but timed out before it could be granted. This value is never
     * returned by the LockManager methods here, but by the Locker class, which offers
     * capability to block while waiting for locks.
     * ������ȴ������ڱ�����֮ǰ��ʱ�������LockManager�������᷵�����ֵ��������Locker�෵�أ���
     * ���ṩ���ڵȴ���ʱ���������Ĺ��ܡ�
     */
    LOCK_TIMEOUT,

    /**
     * The lock request was not granted because it would result in a deadlock. No changes to
     * the state of the Locker would be made if this value is returned (i.e., it will not be
     * killed due to deadlock). It is up to the caller to decide how to recover from this
     * return value - could be either release some locks and try again, or just bail with an
     * error and have some upper code handle it.
     * û��������������Ϊ��ᵼ��������������ش�ֵ���򲻻����״̬(������������Ϊ��������ɱ��)��
     * ��δ��������ֵ�лָ�ȡ���ڵ����ߡ��������ͷ�һЩ�������ԣ�Ҳ�����ͷ�һ��������һЩ�߼����봦������
     */
    LOCK_DEADLOCK,

    /**
     * This is used as an initialiser value. Should never be returned.
     */
    LOCK_INVALID
};


/**
 * Hierarchy of resource types. The lock manager knows nothing about this hierarchy, it is
 * purely logical. Resources of different types will never conflict with each other.
 *
 * While the lock manager does not know or care about ordering, the general policy is that
 * resources are acquired in the order below. For example, one might first acquire a
 * RESOURCE_GLOBAL and then the desired RESOURCE_DATABASE, both using intent modes, and
 * finally a RESOURCE_COLLECTION in exclusive mode. When locking multiple resources of the
 * same type, the canonical order is by resourceId order.
 *
 * It is OK to lock resources out of order, but it is the users responsibility to ensure
 * ordering is consistent so deadlock cannot occur.
 */
/*
db/concurrency/lock_state.cpp:const ResourceId resourceIdGlobal = ResourceId(RESOURCE_GLOBAL, ResourceId::SINGLETON_GLOBAL);
db/concurrency/lock_state.cpp:const ResourceId resourceIdLocalDB = ResourceId(RESOURCE_DATABASE, StringData("local"));
db/concurrency/lock_state.cpp:const ResourceId resourceIdOplog = ResourceId(RESOURCE_COLLECTION, StringData("local.oplog.rs"));
db/concurrency/lock_state.cpp:const ResourceId resourceIdAdminDB = ResourceId(RESOURCE_DATABASE, StringData("admin"));
*/
//��ͬ���͵���ͳ����LockStats��ʵ��
enum ResourceType { //ResourceType::getType
    // Types used for special resources, use with a hash id from ResourceId::SingletonHashIds.
    RESOURCE_INVALID = 0,
    //ȫ����
    RESOURCE_GLOBAL,        // Used for mode changes or global exclusive operations
    RESOURCE_MMAPV1_FLUSH,  // Necessary only for the MMAPv1 engine

    // Generic resources, used for multi-granularity locking, together with RESOURCE_GLOBAL
    RESOURCE_DATABASE, //����
    RESOURCE_COLLECTION, //����
    RESOURCE_METADATA,

    // Resource type used for locking general resources not related to the storage hierarchy.
    RESOURCE_MUTEX,

    // Counts the rest. Always insert new resource types above this entry.
    ResourceTypesCount
};

/**
 * Returns a human-readable name for the specified resource type.
 */
const char* resourceTypeName(ResourceType resourceType);

/**
 * Uniquely identifies a lockable resource.
 */ //resourceIdGlobalȫ�ֱ���Ϊ��������
 /*
db/concurrency/lock_state.cpp:const ResourceId resourceIdGlobal = ResourceId(RESOURCE_GLOBAL, ResourceId::SINGLETON_GLOBAL);
db/concurrency/lock_state.cpp:const ResourceId resourceIdLocalDB = ResourceId(RESOURCE_DATABASE, StringData("local"));
db/concurrency/lock_state.cpp:const ResourceId resourceIdOplog = ResourceId(RESOURCE_COLLECTION, StringData("local.oplog.rs"));
db/concurrency/lock_state.cpp:const ResourceId resourceIdAdminDB = ResourceId(RESOURCE_DATABASE, StringData("admin"));

const ResourceId resourceIdGlobal = ResourceId(RESOURCE_GLOBAL, ResourceId::SINGLETON_GLOBAL);
const ResourceId resourceIdParallelBatchWriterMode =
    ResourceId(RESOURCE_GLOBAL, ResourceId::SINGLETON_PARALLEL_BATCH_WRITER_MODE);

*/ //ȫ���� ���� �����ֱ���Ӧһ������ṹ����ResourceType���
//ResourceId��(����ȫ���� ���� ����)��ÿ��ResourceId������ϸ��Ϊ��ͬ���͵�MODE_IS MODE_IX MODE_S MODE_X��

//LockerImpl._requests map��Ϊ������, ResourceId���뵽��map����
class ResourceId {
    // We only use 3 bits for the resource type in the ResourceId hash
    enum { resourceTypeBits = 3 };
    MONGO_STATIC_ASSERT(ResourceTypesCount <= (1 << resourceTypeBits));

public:
    /**
     * Assign hash ids for special resources to avoid accidental reuse of ids. For ids used
     * with the same ResourceType, the order here must be the same as the locking order.
     */
    /*
    const ResourceId resourceIdGlobal = ResourceId(RESOURCE_GLOBAL, ResourceId::SINGLETON_GLOBAL);
    const ResourceId resourceIdParallelBatchWriterMode =
        ResourceId(RESOURCE_GLOBAL, ResourceId::SINGLETON_PARALLEL_BATCH_WRITER_MODE);
    */
    enum SingletonHashIds {
        SINGLETON_INVALID = 0,
        //��ͬ����أ��ο�Lock::ParallelBatchWriterMode::ParallelBatchWriterMode
        SINGLETON_PARALLEL_BATCH_WRITER_MODE,
        SINGLETON_GLOBAL,
        SINGLETON_MMAPV1_FLUSH,
    };

    ResourceId() : _fullHash(0) {}
    ResourceId(ResourceType type, StringData ns);
    ResourceId(ResourceType type, const std::string& ns);
    ResourceId(ResourceType type, uint64_t hashId);

    bool isValid() const {
        return getType() != RESOURCE_INVALID;
    }

    operator uint64_t() const {
        return _fullHash;
    }

    // This defines the canonical locking order, first by type and then hash id
    bool operator<(const ResourceId& rhs) const {
        return _fullHash < rhs._fullHash;
    }

    //ResourceId::toString���ã����ResourceId::fullHash�Ķ�              
    ResourceType getType() const {
        return static_cast<ResourceType>(_fullHash >> (64 - resourceTypeBits));
    }

    uint64_t getHashId() const {
        return _fullHash & (std::numeric_limits<uint64_t>::max() >> resourceTypeBits);
    }

    std::string toString() const;

private:
    /**
     * The top 'resourceTypeBits' bits of '_fullHash' represent the resource type,
     * while the remaining bits contain the bottom bits of the hashId. This avoids false
     * conflicts between resources of different types, which is necessary to prevent deadlocks.
     */ //�㷨��ResourceId::ResourceId(ResourceType type, uint64_t hashId)
    uint64_t _fullHash;

    static uint64_t fullHash(ResourceType type, uint64_t hashId);

#ifdef MONGO_CONFIG_DEBUG_BUILD
    // Keep the complete namespace name for debugging purposes (TODO: this will be
    // removed once we are confident in the robustness of the lock manager).
    std::string _nsCopy;
#endif
};

#ifndef MONGO_CONFIG_DEBUG_BUILD
// Treat the resource ids as 64-bit integers in release mode in order to ensure we do
// not spend too much time doing comparisons for hashing.
MONGO_STATIC_ASSERT(sizeof(ResourceId) == sizeof(uint64_t));
#endif


// Type to uniquely identify a given locker object
//LockerImpl._idΪ����  ÿ��LockerImpl�����һ��Ψһ��id��ʶ
typedef uint64_t LockerId;

// Hardcoded resource id for the oplog collection, which is special-cased both for resource
// acquisition purposes and for statistics reporting.
extern const ResourceId resourceIdLocalDB;
extern const ResourceId resourceIdOplog;

// Hardcoded resource id for admin db. This is to ensure direct writes to auth collections
// are serialized (see SERVER-16092)
extern const ResourceId resourceIdAdminDB;

// Hardcoded resource id for ParallelBatchWriterMode. We use the same resource type
// as resourceIdGlobal. This will also ensure the waits are reported as global, which
// is appropriate. The lock will never be contended unless the parallel batch writers
// must stop all other accesses globally. This resource must be locked before all other
// resources (including resourceIdGlobal). Replication applier threads don't take this
// lock.
// TODO: Merge this with resourceIdGlobal
extern const ResourceId resourceIdParallelBatchWriterMode;

/**
 * Interface on which granted lock requests will be notified. See the contract for the notify
 * method for more information and also the LockManager::lock call.
 * �������ӿ������ڸýӿ��ϵõ�֪ͨ���йظ�����Ϣ���Լ�LockManager::lock���ã���μ�notify������
 *
 * The default implementation of this method would simply block on an event until notify has
 * been invoked (see CondVarLockGrantNotification).
 *
 * Test implementations could just count the number of notifications and their outcome so that
 * they can validate locks are granted as desired and drive the test execution.
 * �˷�����Ĭ��ʵ�ֽ��򵥵������¼���ֱ������notify(�����CondVarLockGrantNotification)��
 * ����ʵ�ֿ���ֻ����֪ͨ���������������������ǾͿ��Ը�����Ҫ��֤���������������������ִ�С�
 */
class LockGrantNotification {
public:
    virtual ~LockGrantNotification() {}

    /**
     * This method is invoked at most once for each lock request and indicates the outcome of
     * the lock acquisition for the specified resource id.
     *
     * Cases where it won't be called are if a lock acquisition (be it in waiting or converting
     * state) is cancelled through a call to unlock.
     *
     * IMPORTANT: This callback runs under a spinlock for the lock manager, so the work done
     *            inside must be kept to a minimum and no locks or operations which may block
     *            should be run. Also, no methods which call back into the lock manager should
     *            be invoked from within this methods (LockManager is not reentrant).
     *
     * @resId ResourceId for which a lock operation was previously called.
     * @result Outcome of the lock operation.
     */
    virtual void notify(ResourceId resId, LockResult result) = 0;
};


/**
 * There is one of those entries per each request for a lock. They hang on a linked list off
 * the LockHead or off a PartitionedLockHead and also are in a map for each Locker. This
 * structure is not thread-safe.
 * ÿ����������һ����������Ŀ�����ǹ���LockHead��PartitionedLockHead�������ϣ�Ҳ��ÿ��Locker��map���ϡ�
 * ����ṹ�����̰߳�ȫ�ġ�
 *
 * LockRequest are owned by the Locker class and it controls their lifetime. They should not
 * be deleted while on the LockManager though (see the contract for the lock/unlock methods).
 * LockRequest����Locker�࣬���������ǵ������ڡ����ǣ���LockManager�ϲ�Ӧ��ɾ������(�����lock/unlock����)��
 */ 
 //LockerImpl._requests map��Ϊ������, ResourceId���뵽��map����  
//LockRequest::initNew�й������
struct LockRequest { // һ��Locker��Ӧһ��LockRequest�࣬LockRequest���и�����ṹ����������locker��������
    enum Status { //status���ַ���ת����LockRequestStatusNames
        STATUS_NEW, //��ʼ״̬
        STATUS_GRANTED, //��Ȩ״̬����ֵ��newRequest
        STATUS_WAITING, //��ͻ����Ҫ�ȴ�����ֵ��newRequest
        STATUS_CONVERTING,
        
        // Counts the rest. Always insert new status types above this entry.
        //��LockRequestStatusNames
        StatusCount
    };

    /**
     * Used for initialization of a LockRequest, which might have been retrieved from cache.
     */
    void initNew(Locker* locker, LockGrantNotification* notify);

    // This is the Locker, which created this LockRequest. Pointer is not owned, just referenced.
    // Must outlive the LockRequest.
    //
    // Written at construction time by Locker
    // Read by LockManager on any thread
    // No synchronization
    Locker* locker;

    // Notification to be invoked when the lock is granted. Pointer is not owned, just referenced.
    // If a request is in the WAITING or CONVERTING state, must live at least until
    // LockManager::unlock is cancelled or the notification has been invoked.
    //
    // Written at construction time by Locker
    // Read by LockManager
    // No synchronization
    LockGrantNotification* notify;

    // If the request cannot be granted right away, whether to put it at the front or at the end of
    // the queue. By default, requests are put at the back. If a request is requested to be put at
    // the front, this effectively bypasses fairness. Default is FALSE.
    //
    // Written at construction time by Locker
    // Read by LockManager on any thread
    // No synchronization
    bool enqueueAtFront; //LockerImpl<IsForMMAPV1>::lockBegin����Ϊtrue

    // When this request is granted and as long as it is on the granted queue, the particular
    // resource's policy will be changed to "compatibleFirst". This means that even if there are
    // pending requests on the conflict queue, if a compatible request comes in it will be granted
    // immediately. This effectively turns off fairness.
    //
    // Written at construction time by Locker
    // Read by LockManager on any thread
    // No synchronization 
    //ȫ����������������Ϊmode == MODE_S || mode == MODE_X
    bool compatibleFirst; //LockerImpl<IsForMMAPV1>::lockBegin����Ϊtrue

    // When set, an attempt is made to execute this request using partitioned lockheads. This speeds
    // up the common case where all requested locking modes are compatible with each other, at the
    // cost of extra overhead for conflicting modes.
    //
    // Written at construction time by LockManager
    // Read by LockManager on any thread
    // No synchronization
    //LockManager::lock    LockRequest::initNew�и�ֵ
    //request->partitioned = (mode == MODE_IX || mode == MODE_IS);   ��������ֵ�Ż�Ϊtrue
    bool partitioned;

    // How many times has LockManager::lock been called for this request. Locks are released when
    // their recursive count drops to zero.
    //
    // Written by LockManager on Locker thread
    // Read by LockManager on Locker thread
    // Read by Locker on Locker thread
    // No synchronization
    unsigned recursiveCount;

    // Pointer to the lock to which this request belongs, or null if this request has not yet been
    // assigned to a lock or if it belongs to the PartitionedLockHead for locker (in which case
    // partitionedLock must be set). The LockHead should be alive as long as there are LockRequests
    // on it, so it is safe to have this pointer hanging around.
    //
    // Written by LockManager on any thread
    // Read by LockManager on any thread
    // Protected by LockHead bucket's mutex
    //��ֵ��newRequest
    LockHead* lock;

    // Pointer to the partitioned lock to which this request belongs, or null if it is not
    // partitioned. Only one of 'lock' and 'partitionedLock' is non-NULL, and a request can only
    // transition from 'partitionedLock' to 'lock', never the other way around.
    //
    // Written by LockManager on any thread
    // Read by LockManager on any thread
    // Protected by LockHead bucket's mutex
    PartitionedLockHead* partitionedLock; 

    // The linked list chain on which this request hangs off the owning lock head. The reason
    // intrusive linked list is used instead of the std::list class is to allow for entries to be
    // removed from the middle of the list in O(1) time, if they are known instead of having to
    // search for them and we cannot persist iterators, because the list can be modified while an
    // iterator is held.
    //
    // Written by LockManager on any thread
    // Read by LockManager on any thread
    // Protected by LockHead bucket's mutex
    LockRequest* prev;
    LockRequest* next;

    // The current status of this request. Always starts at STATUS_NEW.
    //
    // Written by LockManager on any thread
    // Read by LockManager on any thread
    // Protected by LockHead bucket's mutex
    Status status;

    // If this request is not granted, the mode which has been requested for this lock. If granted,
    // the mode in which it is currently granted.
    //
    // Written by LockManager on any thread
    // Read by LockManager on any thread
    // Protected by LockHead bucket's mutex
    //��Ӧ��LockMode
    LockMode mode;

    // This value is different from MODE_NONE only if a conversion is requested for a lock and that
    // conversion cannot be immediately granted.
    //
    // Written by LockManager on any thread
    // Read by LockManager on any thread
    // Protected by LockHead bucket's mutex
    LockMode convertMode;
};

/**
 * Returns a human readable status name for the specified LockRequest status.
 */
const char* lockRequestStatusName(LockRequest::Status status);

}  // namespace mongo


MONGO_HASH_NAMESPACE_START
template <>
struct hash<mongo::ResourceId> {
    size_t operator()(const mongo::ResourceId& resource) const {
        return resource;
    }
};
MONGO_HASH_NAMESPACE_END
