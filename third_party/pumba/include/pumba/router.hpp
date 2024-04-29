#pragma once

#include <map>
#include <list>
#include <stdexcept>

namespace pumba {

template<typename NodeType, typename HashTraits>
class ConsistentHashingRouter {
public:

    using HashType = typename HashTraits::HashType;
    using NodeIt = typename std::list<NodeType>::iterator;

public:

    explicit ConsistentHashingRouter(std::size_t replicas = 1)
        : replicas_(replicas)
    {}

    template<typename... Args>
    void EmplaceNode(Args&&... args)
    {
        physical_nodes_.emplace_back(std::forward<Args>(args)...);
        NodeIt it = std::prev(physical_nodes_.end());
        SpawnReplicas(it);
    }

    void InsertNode(NodeType&& node)
    {
        physical_nodes_.push_back(node);
        NodeIt it = std::prev(physical_nodes_.end());
        SpawnReplicas(it);
    }

    void EraseNode(NodeType&& node) {
        if (auto it = ring_.find(HashTraits::GetHash(node)); it != ring_.end()) {
            NodeIt node_it = it->second;
            EraseReplicasOf(node_it);
            physical_nodes_.erase(node_it);
        }
    }

    void EraseNode(const NodeType& node) {
        if (auto it = ring_.find(HashTraits::GetHash(node)); it != ring_.end()) {
            NodeIt node_it = it->second;
            EraseReplicasOf(node_it);
            physical_nodes_.erase(node_it);
        }
    }

    const NodeType& SelectNode(const NodeType& client)
    {
        if (Empty()) {
            throw std::runtime_error("pumba::Router: no nodes on the ring");
        }

        HashType hash = HashTraits::GetHash(client);
        auto result_it = ring_.lower_bound(hash);
        if (result_it == ring_.end()) {
            result_it = ring_.begin();
        }
        return *(result_it->second);
    }

    std::size_t PhysicalNodes() const noexcept
    {
        return physical_nodes_.size();
    }

    bool Empty() const noexcept
    {
        return physical_nodes_.empty();
    }

    std::size_t Replicas() const noexcept
    {
        return replicas_;
    }
private:

    void EraseReplicasOf(NodeIt physical_node)
    {
        for (auto it = ring_.begin(); it != ring_.end();) {
            if (it->second == physical_node) {
                it = ring_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void SpawnReplicas(NodeIt physical_node)
    {
        for (const HashType& hash : HashTraits::Replicate(*physical_node, replicas_)) {
            ring_[hash] = physical_node;
        }
    }

private:
    std::map<HashType, NodeIt> ring_;
    std::list<NodeType> physical_nodes_;
    std::size_t replicas_;
};

} // namespace router