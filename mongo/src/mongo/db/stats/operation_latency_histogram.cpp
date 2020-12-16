/**
 * Copyright (C) 2016 MongoDB Inc.
 *
 * This program is free software: you can redistribute it and/or  modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, the copyright holders give permission to link the
 * code of portions of this program with the OpenSSL library under certain
 * conditions as described in each individual source file and distribute
 * linked combinations including the program with the OpenSSL library. You
 * must comply with the GNU Affero General Public License in all respects
 * for all of the code used other than as permitted herein. If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so. If you do not
 * wish to do so, delete this exception statement from your version. If you
 * delete this exception statement from all source files in the program,
 * then also delete it in the license file.
 */

#include "mongo/platform/basic.h"

#include "mongo/db/stats/operation_latency_histogram.h"

#include <algorithm>

#include "mongo/bson/bsonobjbuilder.h"
#include "mongo/db/namespace_string.h"
#include "mongo/platform/bits.h"

namespace mongo {

//��ʱ�Ӱ�����Щά�Ȳ��Ϊ��ͬ���� _getBucket���ж�ʱ��ͳ��Ӧ�������Ǹ�����
const std::array<uint64_t, OperationLatencyHistogram::kMaxBuckets>
    OperationLatencyHistogram::kLowerBounds = {0,
                                               2,
                                               4,
                                               8,
                                               16,
                                               32,
                                               64,
                                               128,
                                               256,
                                               512,
                                               1024,
                                               2048,
                                               3072,
                                               4096,
                                               6144,
                                               8192,
                                               12288,
                                               16384,
                                               24576,
                                               32768,
                                               49152,
                                               65536,
                                               98304,
                                               131072,
                                               196608,
                                               262144,
                                               393216,
                                               524288,
                                               786432,
                                               1048576,
                                               1572864,
                                               2097152,
                                               4194304,
                                               8388608,
                                               16777216,
                                               33554432,
                                               67108864,
                                               134217728,
                                               268435456,
                                               536870912,
                                               1073741824,
                                               2147483648,
                                               4294967296,
                                               8589934592,
                                               17179869184,
                                               34359738368,
                                               68719476736,
                                               137438953472,
                                               274877906944,
                                               549755813888,
                                               1099511627776};


/*
ocloud_oFEAkecX_shard_1:PRIMARY> db.collection.latencyStats( { histograms:true}).pretty()
{
        "ns" : "cloud_track.collection",
        "shard" : "ocloud_oFEAkecX_shard_1",
        "host" : "bjcp1134:20015",
        "localTime" : ISODate("2020-12-12T11:26:51.790Z"),
        "latencyStats" : {
                "reads" : {
                        "histogram" : [
                                {
                                        "micros" : NumberLong(16),
                                        "count" : NumberLong(6)
                                },
                                {
                                        "micros" : NumberLong(32),
                                        "count" : NumberLong(19)
                                },
                                {
                                        "micros" : NumberLong(64),
                                        "count" : NumberLong(1)
                                },
                                {
                                        "micros" : NumberLong(512),
                                        "count" : NumberLong(1)
                                },
                                {
                                        "micros" : NumberLong(3072),
                                        "count" : NumberLong(1)
                                }
                        ],
                        "latency" : NumberLong(5559),
                        "ops" : NumberLong(28)
                },
                "writes" : {
                        "histogram" : [ ],
                        "latency" : NumberLong(0),
                        "ops" : NumberLong(0)
                },
                "commands" : {
                        "histogram" : [ ],
                        "latency" : NumberLong(0),
                        "ops" : NumberLong(0)
                }
        }
}
*/
//OperationLatencyHistogram::append����
void OperationLatencyHistogram::_append(const HistogramData& data,
                                        const char* key,
                                        bool includeHistograms,
                                        BSONObjBuilder* builder) const {

    BSONObjBuilder histogramBuilder(builder->subobjStart(key));
    if (includeHistograms) {
        BSONArrayBuilder arrayBuilder(histogramBuilder.subarrayStart("histogram"));
        for (int i = 0; i < kMaxBuckets; i++) {
            if (data.buckets[i] == 0)
                continue;
            BSONObjBuilder entryBuilder(arrayBuilder.subobjStart());
            entryBuilder.append("micros", static_cast<long long>(kLowerBounds[i]));
            entryBuilder.append("count", static_cast<long long>(data.buckets[i]));
            entryBuilder.doneFast();
        }
        arrayBuilder.doneFast();
    }
    histogramBuilder.append("latency", static_cast<long long>(data.sum));
    histogramBuilder.append("ops", static_cast<long long>(data.entryCount));
    histogramBuilder.doneFast();
}

//��дdb.serverStatus().opLatencies������ؼ��������б��ͳ�� ---ȫ��γ��
//db.collection.latencyStats( { histograms:true})  --- ��γ��
//db.collection.latencyStats( { histograms:false}) --- ��γ��

//Top::appendGlobalLatencyStats����
void OperationLatencyHistogram::append(bool includeHistograms, BSONObjBuilder* builder) const {
    _append(_reads, "reads", includeHistograms, builder);
    _append(_writes, "writes", includeHistograms, builder);
    _append(_commands, "commands", includeHistograms, builder);
}

/*
histogram: [
  { micros: NumberLong(1), count: NumberLong(10) },
  { micros: NumberLong(2), count: NumberLong(1) },
  { micros: NumberLong(4096), count: NumberLong(1) },
  { micros: NumberLong(16384), count: NumberLong(1000) },
  { micros: NumberLong(49152), count: NumberLong(100) }
]
This indicates that there were:

10 operations taking 1 microsecond or less,    10������ʱ��С��1ms
1 operation in the range (1, 2] microseconds,   1������ʱ�ӷ�Χ��1-2��
1 operation in the range (3072, 4096] microseconds, 1������ʱ�ӷ�Χ��3072-4096��
1000 operations in the range (12288, 16384], and  1000������ʱ�ӷ�Χ��12288-16384��
100 operations in the range (32768, 49152].  100������ʱ�ӷ�Χ��32768-49152��
*/

//��¼��ͬʱ�������־����ϸͳ��
//ȷ��latencyʱ�Ӷ�Ӧ��[0-2]��(2-4]��(4-8]��(8-16]��(16-32]��(32-64]��(64-128]...�е��Ǹ�����  
//���������ֱ��ӦbucketsͰ0��Ͱ1��Ͱ2��Ͱ3�ȴ���Ҳ����[0-2]��ӦͰ0��(2-4]��ӦͰ1��(4-8]��ӦͰ2��(8-16]��ӦͰ3����������
// Computes the log base 2 of value, and checks for cases of split buckets.
int OperationLatencyHistogram::_getBucket(uint64_t value) {
    // Zero is a special case since log(0) is undefined.
    if (value == 0) {
        return 0;
    }

    int log2 = 63 - countLeadingZeros64(value);
    // Half splits occur in range [2^11, 2^21) giving 10 extra buckets.
    if (log2 < 11) {
        return log2;
    } else if (log2 < 21) {
        int extra = log2 - 11;
        // Split value boundary is at (2^n + 2^(n+1))/2 = 2^n + 2^(n-1).
        // Which corresponds to (1ULL << log2) | (1ULL << (log2 - 1))
        // Which is equivalent to the following:
        uint64_t splitBoundary = 3ULL << (log2 - 1);
        if (value >= splitBoundary) {
            extra++;
        }
        return log2 + extra;
    } else {
        // Add all of the extra 10 buckets.
        return std::min(log2 + 10, kMaxBuckets - 1);
    }
}

//OperationLatencyHistogram::increment�е���
//�� д command�ܲ���������ʱ�Ӷ�Ӧ����latency
void OperationLatencyHistogram::_incrementData(uint64_t latency, int bucket, HistogramData* data) {
    //����bucketͰָ��ʱ�ӷ�Χ�Ķ�Ӧ����������
	data->buckets[bucket]++;
	//�ò����ܼ���
    data->entryCount++;
	//�ò�����ʱ�Ӽ���
    data->sum += latency;
}

/*
db.serverStatus().opLatencies(ȫ��γ��) 
db.collection.latencyStats( { histograms:true}).pretty()(��γ��) 
*/
//Top._globalHistogramStatsȫ��(�������б�)�Ĳ�����ʱ��ͳ��-ȫ��γ��
//CollectionData.opLatencyHistogram�Ǳ���Ķ���д��commandͳ��-��γ��

//��ͬ�������ο�getReadWriteType
//Top::_incrementHistogram   ������ʱ�Ӽ�������
void OperationLatencyHistogram::increment(uint64_t latency, Command::ReadWriteType type) {
	//ȷ��latencyʱ�Ӷ�Ӧ��[0-2]��(2-4]��(4-8]��(8-16]��(16-32]��(32-64]��(64-128]...�е��Ǹ�����??
	int bucket = _getBucket(latency);
    switch (type) {
		//��ʱ���ۼӣ�������������
        case Command::ReadWriteType::kRead:
            _incrementData(latency, bucket, &_reads);
            break;
		//дʱ���ۼӣ�������������
        case Command::ReadWriteType::kWrite:
            _incrementData(latency, bucket, &_writes);
            break;
		//commandʱ���ۼӣ�������������
        case Command::ReadWriteType::kCommand:
            _incrementData(latency, bucket, &_commands);
            break;
        default:
            MONGO_UNREACHABLE;
    }
}

}  // namespace mongo
