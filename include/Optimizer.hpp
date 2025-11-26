// namespace OptiProSimple
//
// Created by fjasis on 11/13/25.
//

#pragma once
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <functional>
#include <memory>
#include "types.hpp"


namespace OptiProSimple {

    using ::OperationID;
    using ::PartID;
    using ::MachineID;

    struct OptMachine {
        MachineID machine_id = -1;
        bool available = true;
        double available_time = 0.0;
    };

    struct ScheduledOp {
        OperationID op_id = -1;
        MachineID machine_id = -1;
        double start = 0.0;
        double end = 0.0;
    };

    struct Arc {
        int src_idx = -1;
        int tgt_idx = -1;
    };

    struct Node {
        OperationID opid = -1;
        std::vector<int> succ; 
        std::vector<int> pred; 
    };

    struct Graph {
        std::vector<std::unique_ptr<Node>> nodes;
        std::vector<std::unique_ptr<Arc>> arcs;
        
        std::unordered_map<OperationID, int> opid_to_index;

        void clear() {
            nodes.clear();
            arcs.clear();
            opid_to_index.clear();
        }
        
        Node* insert_node(OperationID opid) {
            auto n = std::make_unique<Node>();
            n->opid = opid;
            int idx = static_cast<int>(nodes.size());
            nodes.push_back(std::move(n));
            opid_to_index[opid] = idx;
            return nodes[idx].get();
        }
        
        Arc* insert_arc(Node* u, Node* v) {
            if (!u || !v) return nullptr;
            int ui = -1, vi = -1;
            for (int i = 0; i < (int)nodes.size(); ++i) {
            if (nodes[i].get() == u) ui = i;
            if (nodes[i].get() == v) vi = i;
            if (ui >= 0 && vi >= 0) break;
            }
            if (ui < 0 || vi < 0) return nullptr;
            auto a = std::make_unique<Arc>();
            a->src_idx = ui;
            a->tgt_idx = vi;
            nodes[ui]->succ.push_back(vi);
            nodes[vi]->pred.push_back(ui);
            arcs.push_back(std::move(a));
            return arcs.back().get();
        }
    };
    inline void build_graph_from_state(const ProductionState &state, Graph &g) {
    
        g.clear();

        for (const auto & [partId, part] : state.parts) {
            for (OperationID opid : part.operations) {
            g.insert_node(opid);
            }
        }

        for (const auto & [partId, part] : state.parts) {
            for (size_t i = 1; i < part.operations.size(); ++i) {
            OperationID prev = part.operations[i-1];
            OperationID cur = part.operations[i];
            int u = g.opid_to_index[prev];
            int v = g.opid_to_index[cur];
            g.insert_arc(g.nodes[u].get(), g.nodes[v].get());
            }
        }
    }

    // Schedule using ProductionState to determine durations and compatible machines
    inline std::vector<ScheduledOp> schedule_orders(Graph &g, std::vector<OptMachine> &machines, const ProductionState &state, double start_time = 0.0, const std::unordered_map<int, double> &completed_node_end = {})
    {
    std::vector<ScheduledOp> schedule;
    int n = (int)g.nodes.size();

    std::vector<int> indeg(n, 0);
    for (int i = 0; i < n; ++i) {
        if (completed_node_end.find(i) != completed_node_end.end()) continue;
        indeg[i] = (int)g.nodes[i]->pred.size();
        for (int p : g.nodes[i]->pred) {
        if (completed_node_end.find(p) != completed_node_end.end()) indeg[i]--;
        }
    }

    std::queue<int> q;
    for (int i = 0; i < n; ++i){
        if (completed_node_end.find(i) == completed_node_end.end() && indeg[i] == 0) q.push(i);
    }

    // for quick machine lookup by id
    std::unordered_map<MachineID, int> machine_index;
    for (int i = 0; i < (int)machines.size(); ++i) machine_index[machines[i].machine_id] = i;

    for (auto &m : machines) if (m.available) m.available_time = std::max(m.available_time, start_time);

    // map node -> end_time (for preds)
    std::vector<double> node_end(n, -1.0);
    for (const auto &kv : completed_node_end) {
        int node_idx = kv.first;
        if (node_idx >= 0 && node_idx < n){
        node_end[node_idx] = kv.second;
        }    
    }

    while (!q.empty()) {
        int idx = q.front(); q.pop();
        OperationID opid = g.nodes[idx]->opid;
        const auto &op = state.operations.at(opid);

        // compute duration (use totalTime if available)
        double duration = static_cast<double>(op.totalTime);

        // find compatible machines for this operation
        std::vector<MachineID> allowed_machines;
        for (const auto & [mid, mdata] : state.machines) {
        if (mdata.machineType != op.requiredMachine) continue;
        // check required specs subset
        bool ok = true;
        for (const auto &spec : op.requiredMachineSpces) {
            if (mdata.machineSpecs.find(spec) == mdata.machineSpecs.end()) { ok = false; break; }
        }
        if (ok) allowed_machines.push_back(mid);
        }

        // choose machine with earliest start
        MachineID chosen_mid = -1; double best_start = 1e300; int chosen_mi = -1;
        for (MachineID mid : allowed_machines) {
        auto it = machine_index.find(mid);
        if (it == machine_index.end()) continue;
        int mi = it->second;
        auto &optm = machines[mi];
        if (!optm.available) continue;
        double machine_av = optm.available_time;
        double pred_max = 0.0;
        for (int p : g.nodes[idx]->pred) if (node_end[p] >= 0) pred_max = std::max(pred_max, node_end[p]);
        double candidate = std::max(machine_av, pred_max);
        if (candidate < best_start) { best_start = candidate; chosen_mid = mid; chosen_mi = mi; }
        }

        if (chosen_mid < 0) {
        continue;
        }

        double start = best_start;
        double end = start + duration;
        schedule.push_back(ScheduledOp{opid, chosen_mid, start, end});
        node_end[idx] = end;
        machines[chosen_mi].available_time = end;

        for (int s : g.nodes[idx]->succ) {
        if (completed_node_end.find(s) != completed_node_end.end()) continue;
        indeg[s]--;
        if (indeg[s] == 0) q.push(s);
        }
    }

    return schedule;
    }

    // Handler de fallo de máquina: marca caída y replanifica desde now
    inline std::vector<ScheduledOp> handle_machine_failure(Graph &g, std::vector<OptMachine> &machines, const std::vector<ScheduledOp> &prior_schedule, const ProductionState &state, MachineID failed_machine_id, double now)
    {
    for (auto &m : machines){
        if (m.machine_id == failed_machine_id) {
            m.available = false;
        }    
    }

    std::unordered_map<int, double> completed_node_end;
    for (const auto &s : prior_schedule) {
        if (s.end <= now) {
        auto it = g.opid_to_index.find(s.op_id);
        if (it != g.opid_to_index.end()) completed_node_end[it->second] = s.end;
        }
    }

    for (auto &m : machines) if (m.available) m.available_time = std::max(m.available_time, now);

    return schedule_orders(g, machines, state, now, completed_node_end);
    }

} 



