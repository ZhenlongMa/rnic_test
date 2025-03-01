// MIT License

// Copyright (c) 2021 ByteDance Inc. All rights reserved.
// Copyright (c) 2021 Duke University.  All rights reserved.

// See LICENSE for license information

#include "htn_endpoint.hh"
#include "htn_context.hh"

namespace Htn {

// todo: pick out WQE generation
int htn_endpoint::PostSend(std::vector<htn_region *> &mem_pool, test_qp qp_case,
                            const std::vector<htn_buffer *> &remote_buffer) {
    // todo: static WQE
    struct ibv_send_wr wr_list[kMaxBatch];
    uint32_t batch_size = qp_case.write_num + qp_case.read_num + qp_case.send_recv_num;
    int remote_buf_idx = 0;
    int finish_wr_num = 0;
    int finish_rd_num = 0;
    int finish_sr_num = 0;
    int rbuf_idx = 0;
    struct ibv_sge sge;
    // htn_buffer *buffer;
    for (int i = 0; i < batch_size; i++) {
        wr_list[i].num_sge = 1; // todo
        for (int j = 0; j < wr_list[i].num_sge; j++) {

        }
        // buffer = mem_pool[0]->buffers_.front();
        sge.addr = mem_pool[0]->buffers_.front()->addr_;
        sge.lkey = mem_pool[0]->buffers_.front()->local_key_;
        sge.length = qp_case.data_size;
        bytes_sent_now_ += sge.length;
        msgs_sent_now_++;
        if (finish_wr_num < qp_case.write_num) {
            wr_list[i].opcode = IBV_WR_RDMA_WRITE;
        }
        else if (finish_rd_num < qp_case.read_num) {
            wr_list[i].opcode = IBV_WR_RDMA_READ;
        }
        else if (finish_sr_num < qp_case.send_recv_num) {
            wr_list[i].opcode = IBV_WR_SEND;
        }
        else {
            LOG(ERROR) << "Insufficient case!";
        }
        switch (wr_list[i].opcode) {
            case IBV_WR_RDMA_WRITE_WITH_IMM:
                wr_list[i].imm_data = 0xdeadbeaf;
            case IBV_WR_RDMA_WRITE:
            case IBV_WR_RDMA_READ:
                wr_list[i].wr.rdma.remote_addr = remote_buffer[rbuf_idx]->addr_;
                wr_list[i].wr.rdma.rkey = remote_buffer[rbuf_idx]->remote_key_;
                break;
            case IBV_WR_SEND_WITH_IMM:
                wr_list[i].imm_data = 0xfeedbeee;
            case IBV_WR_SEND:
                if (qp_type_ == IBV_QPT_UD) {
                wr_list[i].wr.ud.remote_qkey = 0;
                wr_list[i].wr.ud.remote_qpn = remote_qpn_;
                wr_list[i].wr.ud.ah = (ibv_ah *)context_;
                }
                break;
            default:
                LOG(ERROR) << "Currently not supporting other operation type: "
                        << wr_list[i].opcode;
                return -1;
        }
        wr_list[i].send_flags = (i == batch_size - 1) ? IBV_SEND_SIGNALED : 0;
        wr_list[i].wr_id = (uint64_t)this;
        wr_list[i].sg_list = &sge;
        wr_list[i].next = (i == batch_size - 1) ? nullptr : &wr_list[i + 1];
    }
    struct ibv_send_wr *bad_wr = nullptr;
    if (ibv_post_send(qp_, wr_list, &bad_wr)) {
        PLOG(ERROR) << "ibv_post_send() failed";
        return -1;
    }
    send_credits_ -= batch_size;
    send_batch_size_.push(batch_size);
    return 0;
}

// int htn_endpoint::PostRecv(const std::vector<rdma_request> &requests,
                            // size_t &req_idx, uint32_t batch_size) {
    // rdma_context *ctx = (rdma_context *)master_;
    // if (recv_credits_ < batch_size) {
    //     LOG(ERROR) << "PostRecv() failed. Credit not available: " << recv_credits_
    //             << " is less than " << batch_size;
    //     return -1;
    // }
    // struct ibv_sge sg[kMaxBatch][kMaxSge];
    // struct ibv_recv_wr wr[kMaxBatch];
    // struct ibv_recv_wr *bad_wr;
    // for (uint32_t i = 0; i < batch_size; i++) {
    //     auto &req = requests[req_idx];
    //     for (int j = 0; j < req.sge_num; j++) {
    //         sg[i][j].addr = req.sglist[j].addr;
    //         sg[i][j].lkey = req.sglist[j].lkey;
    //         sg[i][j].length = req.sglist[j].length + kUdAddition;
    //     }
    //     memset(&wr[i], 0, sizeof(struct ibv_recv_wr));
    //     wr[i].num_sge = req.sge_num;
    //     wr[i].sg_list = sg[i];
    //     wr[i].next = (i == batch_size - 1) ? nullptr : &wr[i + 1];
    //     wr[i].wr_id = reinterpret_cast<uint64_t>(this);
    //     req_idx = (req_idx == requests.size() - 1) ? 0 : req_idx + 1;
    // }
    // if (auto ret = ibv_post_recv(qp_, wr, &bad_wr)) {
    //     PLOG(ERROR) << "ibv_post_recv() failed";
    //     LOG(ERROR) << "Return value is " << ret;
    //     return -1;
    // }
    // recv_credits_ -= batch_size;
    // // No need for recv. Each successful request generates a CQE
    // // recv_batch_size_.push(batch_size);
    // return 0;
// }

// int htn_endpoint::RestoreFromERR() {
    // struct ibv_qp_attr attr;
    // int attr_mask;
    // attr_mask = IBV_QP_STATE;
    // memset(&attr, 0, sizeof(struct ibv_qp_attr));
    // attr.qp_state = IBV_QPS_RESET;
    // if (ibv_modify_qp(qp_, &attr, attr_mask)) {
    //     PLOG(ERROR) << "Failed to restore QP from ERR to RESET";
    //     return -1;
    // }
    // auto remote_gid = remote_gid_;
    // if (Activate(remote_gid)) {
    //     PLOG(ERROR) << "Failed to restore QP to RTS";
    //     return -1;
    // }
    // return 0;
// }

int htn_endpoint::Activate(const union ibv_gid &remote_gid) {
    remote_gid_ = remote_gid;
    struct ibv_qp_attr attr;
    int attr_mask;
    attr = MakeQpAttr(IBV_QPS_INIT, qp_type_, 0, remote_gid, &attr_mask);
    if (ibv_modify_qp(qp_, &attr, attr_mask)) {
        PLOG(ERROR) << "Failed to modify QP to INIT";
        return -1;
    }
    attr = MakeQpAttr(IBV_QPS_RTR, qp_type_, remote_qpn_, remote_gid, &attr_mask);
    if (ibv_modify_qp(qp_, &attr, attr_mask)) {
        PLOG(ERROR) << "Failed to modify QP to RTR";
        return -1;
    }
    attr = MakeQpAttr(IBV_QPS_RTS, qp_type_, remote_qpn_, remote_gid, &attr_mask);
    if (ibv_modify_qp(qp_, &attr, attr_mask)) {
        PLOG(ERROR) << "Failed to modify QP to RTS";
        return -1;
    }
    // if (qp_type_ == IBV_QPT_UD) {
    //     struct ibv_ah_attr ah_attr;
    //     memset(&ah_attr, 0, sizeof(ah_attr));
    //     auto master_ctx = (rdma_context *)master_;
    //     ah_attr.dlid = dlid_;
    //     ah_attr.is_global = 1;
    //     memcpy(&ah_attr.grh.dgid, &remote_gid, sizeof(union ibv_gid));
    //     ah_attr.grh.flow_label = 0;
    //     ah_attr.grh.sgid_index = FLAGS_gid;
    //     ah_attr.grh.hop_limit = FLAGS_hop_limit;
    //     ah_attr.grh.traffic_class = FLAGS_tos;
    //     ah_attr.sl = remote_sl_;
    //     ah_attr.src_path_bits = 0;
    //     ah_attr.port_num = 1;
    //     // [SEVERE TODO]: when scales up, ibv_create_ah may block and failed.
    //     context_ = (void *)ibv_create_ah(master_ctx->GetPd(id_), &ah_attr);
    //     if (!context_) {
    //         PLOG(ERROR) << "ibv_create_ah() failed";
    //         return -1;
    //     }
    // }
    return 0;
}

int htn_endpoint::SendHandler(struct ibv_wc *wc) {
    auto update_credits = send_batch_size_.front();
    send_batch_size_.pop();
    send_credits_ += update_credits;
    // todo: trigger post send
    return 0;
}

// int htn_endpoint::RecvHandler(struct ibv_wc *wc) {
    // // Reply or something else here.
    // // auto update_credits = recv_batch_size_.front();
    // // recv_batch_size_.pop();
    // // recv_credits_ += update_credits;
    // recv_credits_++;
    // return 0;
// }

// void htn_endpoint::PrintThroughput(uint64_t timestamp) {
    // if (bytes_sent_last_ == 0) {
    //     timestamp_ = timestamp;
    //     bytes_sent_last_ = bytes_sent_now_;
    //     msgs_sent_last_ = msgs_sent_now_;
    //     return;
    // }
    // auto t = timestamp - timestamp_;
    // if (t >= 1000000) {  // report every 1s.
    //     auto throughput =
    //         (bytes_sent_now_ - bytes_sent_last_) * 8.0 * 1.0 / t;          // mbps
    //     auto qps = (msgs_sent_now_ - msgs_sent_last_) * 1.0 * 1000.0 / t;  // krps
    //     LOG(INFO) << "conn " << id_ << " " << ((rdma_context *)master_)->GetIp()
    //             << ":" << qp_->qp_num << "-" << remote_server_ << ":"
    //             << remote_qpn_ << " Bytes=" << bytes_sent_now_
    //             << " Rate=" << (int)throughput << " Mbps  ("
    //             << throughput / 1000.0 << " Gbps)";
    //     timestamp_ = timestamp;
    //     LOG(INFO) << "\t\t\t\t"
    //             << " Message rate is " << qps << " Krps (" << qps / 1000.0
    //             << " Mrps)";
    //     bytes_sent_last_ = bytes_sent_now_;
    //     msgs_sent_last_ = msgs_sent_now_;
    // }
    // return;
// }

}  // namespace Collie