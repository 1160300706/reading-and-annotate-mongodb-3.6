/**
 *    Copyright (C) 2015 MongoDB Inc.
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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kSharding

#include "mongo/platform/basic.h"

#include "mongo/s/shard_util.h"

#include "mongo/base/status_with.h"
#include "mongo/bson/util/bson_extract.h"
#include "mongo/client/read_preference.h"
#include "mongo/client/remote_command_targeter.h"
#include "mongo/db/namespace_string.h"
#include "mongo/s/client/shard_registry.h"
#include "mongo/s/grid.h"
#include "mongo/s/shard_key_pattern.h"
#include "mongo/util/log.h"
#include "mongo/util/mongoutils/str.h"

namespace mongo {
namespace shardutil {
namespace {

const char kMinKey[] = "min";
const char kMaxKey[] = "max";
const char kShouldMigrate[] = "shouldMigrate";

}  // namespace

StatusWith<long long> retrieveTotalShardSize(OperationContext* opCtx, const ShardId& shardId) {
    auto shardStatus = Grid::get(opCtx)->shardRegistry()->getShard(opCtx, shardId);
    if (!shardStatus.isOK()) {
        return shardStatus.getStatus();
    }

    // Since 'listDatabases' is potentially slow in the presence of large number of collections, use
    // a higher maxTimeMS to prevent it from prematurely timing out
    const Minutes maxTimeMSOverride{10};

    auto listDatabasesStatus = shardStatus.getValue()->runCommandWithFixedRetryAttempts(
        opCtx,
        ReadPreferenceSetting{ReadPreference::PrimaryPreferred},
        "admin",
        BSON("listDatabases" << 1),
        maxTimeMSOverride,
        Shard::RetryPolicy::kIdempotent);

    if (!listDatabasesStatus.isOK()) {
        return std::move(listDatabasesStatus.getStatus());
    }

    if (!listDatabasesStatus.getValue().commandStatus.isOK()) {
        return std::move(listDatabasesStatus.getValue().commandStatus);
    }

    BSONElement totalSizeElem = listDatabasesStatus.getValue().response["totalSize"];
    if (!totalSizeElem.isNumber()) {
        return {ErrorCodes::NoSuchKey, "totalSize field not found in listDatabases"};
    }

    return totalSizeElem.numberLong();
}

/*
https://blog.csdn.net/weixin_33827731/article/details/90534750
db.runCommand({splitVector:"blog.post", keyPattern:{x:1}, min{x:10}, max:{x:20}, maxChunkSize:200}) 把 10-20这个范围的数据拆分为200个子块
*/ 
//splite命令中得selectMedianKey为强制获取中间分裂点
//发送splitVector命令给指定shard,从对应shard server获取某个chunk的分裂点
//updateChunkWriteStatsAndSplitIfNeeded中调用
StatusWith<std::vector<BSONObj>> 
//返回分裂点
selectChunkSplitPoints(OperationContext* opCtx,
                                                        const ShardId& shardId,
                                                        const NamespaceString& nss,
                                                        const ShardKeyPattern& shardKeyPattern,
                                                        const ChunkRange& chunkRange,
                                                        long long chunkSizeBytes,
                                                        boost::optional<int> maxObjs) {
    BSONObjBuilder cmd;
    cmd.append("splitVector", nss.ns());
    cmd.append("keyPattern", shardKeyPattern.toBSON());
    chunkRange.append(&cmd);
    cmd.append("maxChunkSizeBytes", chunkSizeBytes);
    if (maxObjs) {
        cmd.append("maxChunkObjects", *maxObjs);
    }

	//发送splitVector命令给指定shard server, shard server得主节点
    auto shardStatus = Grid::get(opCtx)->shardRegistry()->getShard(opCtx, shardId);
    if (!shardStatus.isOK()) {
        return shardStatus.getStatus();
    }

    auto cmdStatus = shardStatus.getValue()->runCommandWithFixedRetryAttempts(
        opCtx,
        ReadPreferenceSetting{ReadPreference::PrimaryPreferred},
        "admin",
        cmd.obj(),
        Shard::RetryPolicy::kIdempotent);
    if (!cmdStatus.isOK()) {
        return std::move(cmdStatus.getStatus());
    }
    if (!cmdStatus.getValue().commandStatus.isOK()) {
        return std::move(cmdStatus.getValue().commandStatus);
    }

    const auto response = std::move(cmdStatus.getValue().response);

    std::vector<BSONObj> splitPoints;

    BSONObjIterator it(response.getObjectField("splitKeys"));
    while (it.more()) {
        splitPoints.push_back(it.next().Obj().getOwned());
    }

    return std::move(splitPoints);
}

//通过splitVector获取分裂点
//ShardingCatalogManager::shardCollection->createFirstChunks中调用
//Balancer::_moveChunks->Balancer::_splitOrMarkJumbo中调用
//updateChunkWriteStatsAndSplitIfNeeded中调用(FindAndModifyCmd::run和ClusterWriter::write->splitIfNeede调用)  

//1. balance的时候进行movechunk前需要判断是否为jumbo chunk，如果是则进行splitchunk拆分，参考Balancer::_moveChunks->Balancer::_splitOrMarkJumbo
//2. SplitCollectionCmd::errmsgRun  手动跑splitchunk命令
//3. 写数据流程，ClusterWriter::write->splitIfNeeded->updateChunkWriteStatsAndSplitIfNeeded    
//4. ConfigSvrShardCollectionCommand::run->migrateAndFurtherSplitInitialChunks调用

//该接口也就是shardutil::splitChunkAtMultiplePoints，其他地方通过调用shardutil::splitChunkAtMultiplePoints走到这里

////ClusterWriter::write->splitIfNeeded->updateChunkWriteStatsAndSplitIfNeeded调用
StatusWith<boost::optional<ChunkRange>> 
	splitChunkAtMultiplePoints(
    OperationContext* opCtx,
    const ShardId& shardId,
    const NamespaceString& nss,
    const ShardKeyPattern& shardKeyPattern,
    ChunkVersion collectionVersion,
    const ChunkRange& chunkRange,
    const std::vector<BSONObj>& splitPoints) {
    invariant(!splitPoints.empty());

    const size_t kMaxSplitPoints = 8192;

    if (splitPoints.size() > kMaxSplitPoints) {
        return {ErrorCodes::BadValue,
                str::stream() << "Cannot split chunk in more than " << kMaxSplitPoints
                              << " parts at a time."};
    }

    // Sanity check that we are not attempting to split at the boundaries of the chunk. This check
    // is already performed at chunk split commit time, but we are performing it here for parity
    // with old auto-split code, which might rely on it.
    if (SimpleBSONObjComparator::kInstance.evaluate(chunkRange.getMin() == splitPoints.front())) {
        const std::string msg(str::stream() << "not splitting chunk " << chunkRange.toString()
                                            << ", split point "
                                            << splitPoints.front()
                                            << " is exactly on chunk bounds");
        return {ErrorCodes::CannotSplit, msg};
    }

    if (SimpleBSONObjComparator::kInstance.evaluate(chunkRange.getMax() == splitPoints.back())) {
        const std::string msg(str::stream() << "not splitting chunk " << chunkRange.toString()
                                            << ", split point "
                                            << splitPoints.back()
                                            << " is exactly on chunk bounds");
        return {ErrorCodes::CannotSplit, msg};
    }

    BSONObjBuilder cmd;
    cmd.append("splitChunk", nss.ns());
    cmd.append("from", shardId.toString());
    cmd.append("keyPattern", shardKeyPattern.toBSON());
    cmd.append("epoch", collectionVersion.epoch());
    collectionVersion.appendForCommands(&cmd);  // backwards compatibility with v3.4
    chunkRange.append(&cmd);
    cmd.append("splitKeys", splitPoints);

    BSONObj cmdObj = cmd.obj();

    Status status{ErrorCodes::InternalError, "Uninitialized value"};
    BSONObj cmdResponse;

    auto shardStatus = Grid::get(opCtx)->shardRegistry()->getShard(opCtx, shardId);
    if (!shardStatus.isOK()) {
        status = shardStatus.getStatus();
    } else {
    	//mongos发送splitChunk命令给shard server, shard server收到后在SplitChunkCommand::run->splitChunk中
    	// 想cfg获取分布式锁更新cfg server元数据，这时候获取分布式锁可能会失败
        auto cmdStatus = shardStatus.getValue()->runCommandWithFixedRetryAttempts(
            opCtx,
            ReadPreferenceSetting{ReadPreference::PrimaryOnly},
            "admin",
            cmdObj,
            Shard::RetryPolicy::kNotIdempotent);
        if (!cmdStatus.isOK()) {
            status = std::move(cmdStatus.getStatus());
        } else {
            status = std::move(cmdStatus.getValue().commandStatus);
            cmdResponse = std::move(cmdStatus.getValue().response);
        }
    }

	//这里有可能失败，因为cfg收到splitChunk命令后，需要获取分布式表锁,详见SplitChunkCommand::run
    if (!status.isOK()) {
        log() << "Split chunk " << redact(cmdObj) << " failed" << causedBy(redact(status));
        return {status.code(), str::stream() << "split failed due to " << status.toString()};
    }

    BSONElement shouldMigrateElement;
    status = bsonExtractTypedField(cmdResponse, kShouldMigrate, Object, &shouldMigrateElement);
    if (status.isOK()) {
        auto chunkRangeStatus = ChunkRange::fromBSON(shouldMigrateElement.embeddedObject());
        if (!chunkRangeStatus.isOK()) {
            return chunkRangeStatus.getStatus();
        }

        return boost::optional<ChunkRange>(std::move(chunkRangeStatus.getValue()));
    } else if (status != ErrorCodes::NoSuchKey) {
        warning()
            << "Chunk migration will be skipped because splitChunk returned invalid response: "
            << redact(cmdResponse) << ". Extracting " << kShouldMigrate << " field failed"
            << causedBy(redact(status));
    }

    return boost::optional<ChunkRange>();
}

}  // namespace shardutil
}  // namespace mongo
