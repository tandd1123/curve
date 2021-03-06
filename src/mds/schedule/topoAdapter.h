/*
 *  Copyright (c) 2020 NetEase Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/*
 * Project: curve
 * Created Date: Wed Nov 28 2018
 * Author: lixiaocui
 */

#ifndef SRC_MDS_SCHEDULE_TOPOADAPTER_H_
#define SRC_MDS_SCHEDULE_TOPOADAPTER_H_
#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include "src/mds/topology/topology.h"
#include "src/mds/topology/topology_service_manager.h"
#include "src/mds/topology/topology_stat.h"
#include "src/mds/common/mds_define.h"
#include "proto/topology.pb.h"
#include "proto/heartbeat.pb.h"

using ::curve::mds::topology::ChunkServerIdType;
using ::curve::mds::topology::PoolIdType;
using ::curve::mds::topology::ServerIdType;
using ::curve::mds::topology::ZoneIdType;
using ::curve::mds::topology::EpochType;
using ::curve::mds::topology::CopySetKey;
using ::curve::mds::topology::CopySetIdType;
using ::curve::mds::topology::OnlineState;
using ::curve::mds::topology::Topology;
using ::curve::mds::topology::TopologyServiceManager;
using ::curve::mds::topology::TopologyStat;
using ::curve::mds::topology::ChunkServer;
using ::curve::mds::topology::Server;
using ::curve::mds::topology::LogicalPool;
using ::curve::mds::topology::DiskState;
using ::curve::mds::topology::ChunkServerStatus;
using ::curve::mds::topology::ChunkServerStat;
using ::curve::mds::topology::UNINTIALIZE_ID;
using ::curve::mds::heartbeat::ConfigChangeInfo;
using ::curve::mds::heartbeat::ConfigChangeType;
using ::curve::mds::heartbeat::CopysetStatistics;
using ::curve::mds::heartbeat::ChunkServerStatisticInfo;

namespace curve {
namespace mds {
namespace schedule {

struct PeerInfo {
 public:
    PeerInfo() : id(UNINTIALIZE_ID), port(0) {}
    PeerInfo(ChunkServerIdType id, ZoneIdType zoneId, ServerIdType sid,
            const std::string &ip,
            uint32_t port);
    ChunkServerIdType id;
    ZoneIdType zoneId;
    ServerIdType serverId;
    std::string ip;
    uint32_t port;
};

struct CopySetConf {
 public:
    CopySetConf() = default;
    CopySetConf(const CopySetKey &key, EpochType epoch,
        const std::vector<PeerInfo> &peers, ConfigChangeType type,
        ChunkServerIdType item,
        ChunkServerIdType oldOne = ::curve::mds::topology::UNINTIALIZE_ID);

    CopySetKey id;
    EpochType epoch;
    std::vector<PeerInfo> peers;
    ConfigChangeType type;
    ChunkServerIdType configChangeItem;
    ChunkServerIdType oldOne = ::curve::mds::topology::UNINTIALIZE_ID;
};

struct CopySetInfo {
 public:
    CopySetInfo() : logicalPoolWork(false) {}
    CopySetInfo(CopySetKey id,
                EpochType epoch,
                ChunkServerIdType leader,
                const std::vector<PeerInfo> &peers,
                const ConfigChangeInfo &info,
                const CopysetStatistics &statistics) : logicalPoolWork(false) {
        this->id.first = id.first;
        this->id.second = id.second;
        this->epoch = epoch;
        this->leader = leader;
        this->peers = peers;
        this->configChangeInfo = info;
        this->statisticsInfo = statistics;
    }
    CopySetInfo(const CopySetInfo &in);
    ~CopySetInfo();

    bool ContainPeer(ChunkServerIdType id) const;
    bool HasCandidate() const;
    std::string CopySetInfoStr() const;

    CopySetKey id;
    // 环境初始化的时copyset全部创建完成logicalPool可用,创建过程中不可用
    bool logicalPoolWork;
    EpochType epoch;
    ChunkServerIdType leader;
    std::vector<PeerInfo> peers;

    // TODO(chaojie): candidateInfo 增加到topology中
    PeerInfo candidatePeerInfo;
    ConfigChangeInfo configChangeInfo;
    CopysetStatistics statisticsInfo;
};

struct ChunkServerInfo {
 public:
    ChunkServerInfo() :
        leaderCount(0), diskCapacity(0), diskUsed(0), startUpTime(0) {}
    ChunkServerInfo(const PeerInfo &info, OnlineState state,
                    DiskState diskState, ChunkServerStatus status,
                    uint32_t leaderCount, uint64_t capacity, uint64_t used,
                    const ChunkServerStatisticInfo &statisticInfo);

    bool IsOnline() const;
    bool IsOffline()const;
    bool IsUnstable()const;
    bool IsPendding()const;
    bool IsHealthy()const;

    PeerInfo info;
    uint64_t startUpTime;
    OnlineState state;
    DiskState diskState;
    ChunkServerStatus status;

    uint32_t leaderCount;
    uint64_t diskCapacity;
    uint64_t diskUsed;
    ChunkServerStatisticInfo statisticInfo;
};

/**
 * @brief TopoAdapter为接口, 提供topology相关信息
 */
class TopoAdapter {
 public:
    /**
     * @brief GetLogicalpools 获取集群中逻辑池集合
     *
     * @return 逻辑池列表
     */
    virtual std::vector<PoolIdType> GetLogicalpools() = 0;

    /**
     * @brief GetCopySetInfo 获取指定copyset信息
     *
     * @param[in] id copysetId
     * @param[out] copyset信息
     *
     * @return false-未获取到指定copyset的信息 true-获取成功
     */
    virtual bool GetCopySetInfo(const CopySetKey &id, CopySetInfo *info) = 0;

    /**
     * @brief GetCopySetInfos 获取所有logicalPoolId可用的copyset信息
     *
     * @return copyset信息列表
     */
    virtual std::vector<CopySetInfo> GetCopySetInfos() = 0;

    /**
     * @brief GetCopySetInfosInChunkServer获取指定chunkserver上的copyset信息
     *
     * @param[in] id 指定chunkserverId
     *
     * @return 指定chunkserver上copyset列表
     */
    virtual std::vector<CopySetInfo> GetCopySetInfosInChunkServer(
        ChunkServerIdType id) = 0;

    /**
     * @brief GetCopySetInfosInLogicalPlol获取指定逻辑池中的copyset信息
     *
     * @param[in] lid 指定逻辑池id
     *
     * @return 指定逻辑池中copyset列表
     */
    virtual std::vector<CopySetInfo> GetCopySetInfosInLogicalPool(
        PoolIdType lid) = 0;

    /**
     * @brief GetChunkServerInfo 获取指定chunkserver信息
     *
     * @param[in] id 指定chunkserver id
     * @param[in] info 指定chunkserver的信息
     *
     * @return false-获取失败，true-获取成功
     */
    virtual bool GetChunkServerInfo(
        ChunkServerIdType id, ChunkServerInfo *info) = 0;

    /**
     * @brief GetChunkServerInfos 获取所有chunkserver的信息
     *
     * @return chunkserver信息列表
     */
    virtual std::vector<ChunkServerInfo> GetChunkServerInfos() = 0;

    /**
     * @brief GetChunkServersInLogicalPool 获取指定逻辑池中所有chunkserver
     *
     * @prarm[in] lid 指定逻辑池id
     *
     * @return 指定逻辑池中chunkserver列表
     */
    virtual std::vector<ChunkServerInfo> GetChunkServersInLogicalPool(
        PoolIdType lid) = 0;

    /**
     * @brief GetStandardZoneNumInLogicalPool 获取指定逻辑池中标准zone值
     *
     * @return 指定逻辑池中标准zone值
     */
    virtual int GetStandardZoneNumInLogicalPool(PoolIdType id) = 0;

    /**
     * @brief GetAvgScatterWidthInLogicalPool
     *        获取指定逻辑池中chunkserver平均scatter-width
     *
     * @ param[in] id 逻辑池id
     *
     * @return 指定逻辑池中chunkserver上平均scatter-width的值
     */
    virtual int GetAvgScatterWidthInLogicalPool(PoolIdType id) = 0;

    /**
     * @brief GetStandardReplicaNumInLogicalPool 获取指定逻辑池中标准副本数量
     *
     * @return 指定逻辑池中标准副本数量
     */
    virtual int GetStandardReplicaNumInLogicalPool(PoolIdType id) = 0;

    /**
     * @brief CreateCopySetAtChunkServer 在csID上创建copyset.
     *        raft的add-configuration需要节点上启动raft服务，
     *        所以在下发配置变更命令之前先要通知chunkserver启动copyset的raft服务
     *
     * @param[in] id copyset key
     * @param[in] 在csID上创建copyset
     *
     * @return false-创建失败 true-创建成功
     */
    virtual bool CreateCopySetAtChunkServer(
        CopySetKey id, ChunkServerIdType csID) = 0;

    /**
     * @brief CopySetFromTopoToSchedule 把topology中copyset转化为schedule中的类型
     *
     * @param[in] origin topology中copyset类型
     * @param[out] out shedule中copyset类型
     *
     * @return false-转化失败 true-转化成功
     */
    virtual bool CopySetFromTopoToSchedule(
        const ::curve::mds::topology::CopySetInfo &origin,
        ::curve::mds::schedule::CopySetInfo *out) = 0;

    /**
     * @brief ChunkServerFromTopoToSchedule
     *        把topology中chunkserver转化为schedule中的类型
     *
     * @param[in] origin topology中chunkserver类型
     * @param[out] out shedule中chunkserver类型
     *
     * @return false-转化失败 true-转化成功
     */
    virtual bool ChunkServerFromTopoToSchedule(
        const ::curve::mds::topology::ChunkServer &origin,
        ::curve::mds::schedule::ChunkServerInfo *out) = 0;

    /**
     * @brief GetChunkServerScatterMap 获取指定chunkserver的scatter-width map
     *
     * @param[in] cs 指定chunkserver id
     * @param[out] out scatter-width map, 其中key表示指定chunkserver上的所有copyset //NOLINT
     *             其他副本列表，value表示key上包含指定chunkserver上copyset的个数 //NOLINT
     *  e.g. chunkserver1: copyset1{1,2,3} copyset2{2,3,4} copyset3{4,5,6}
     *       scatter-width map为:
     *       {{2, 2}, {3, 2}, {4, 2}, {5, 1}, {6, 1}}
     *       chunkserver2上有copyset1和copyset2
     *       chunkserver3上有copyset1和copyset2
     *       chunkserver4上有copyset2和copyset3
     *       依次类推
     */
    virtual void GetChunkServerScatterMap(const ChunkServerIdType &cs,
        std::map<ChunkServerIdType, int> *out) = 0;
};

// adapter实现
class TopoAdapterImpl : public TopoAdapter {
 public:
    TopoAdapterImpl() = default;
    explicit TopoAdapterImpl(std::shared_ptr<Topology> topo,
                             std::shared_ptr<TopologyServiceManager> manager,
                             std::shared_ptr<TopologyStat> stat);

    std::vector<PoolIdType> GetLogicalpools() override;

    bool GetCopySetInfo(
        const CopySetKey &id, CopySetInfo *info) override;

    std::vector<CopySetInfo> GetCopySetInfos() override;

    std::vector<CopySetInfo> GetCopySetInfosInChunkServer(
        ChunkServerIdType id) override;

    std::vector<CopySetInfo> GetCopySetInfosInLogicalPool(
        PoolIdType lid) override;

    bool GetChunkServerInfo(
        ChunkServerIdType id, ChunkServerInfo *info) override;

    std::vector<ChunkServerInfo> GetChunkServerInfos() override;

    std::vector<ChunkServerInfo> GetChunkServersInLogicalPool(
        PoolIdType lid) override;

    int GetStandardZoneNumInLogicalPool(PoolIdType id) override;

    int GetStandardReplicaNumInLogicalPool(PoolIdType id) override;

    int GetAvgScatterWidthInLogicalPool(PoolIdType id) override;

    bool CreateCopySetAtChunkServer(
        CopySetKey id, ChunkServerIdType csID) override;

    bool CopySetFromTopoToSchedule(
        const ::curve::mds::topology::CopySetInfo &origin,
        ::curve::mds::schedule::CopySetInfo *out) override;

    bool ChunkServerFromTopoToSchedule(
        const ::curve::mds::topology::ChunkServer &origin,
        ::curve::mds::schedule::ChunkServerInfo *out) override;

    void GetChunkServerScatterMap(const ChunkServerIdType &cs,
        std::map<ChunkServerIdType, int> *out) override;

 private:
    bool GetPeerInfo(ChunkServerIdType id, PeerInfo *peerInfo);

 private:
    std::shared_ptr<Topology> topo_;
    std::shared_ptr<TopologyServiceManager> topoServiceManager_;
    std::shared_ptr<TopologyStat> topoStat_;
};
}  // namespace schedule
}  // namespace mds
}  // namespace curve

#endif  // SRC_MDS_SCHEDULE_TOPOADAPTER_H_
