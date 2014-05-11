/*
 * Multi-Level Feedback Queue (MLFQ)
 * Copyright (c) 2014 Wei Bai (baiwei0427@gmail.com) from Hong Kong University of Science and Technology
 * All rights reserved.
 *
 * In MLFQ, it contains N CoS (Class of Service) queues.
 * MLFQ uses Strict Priority (SP) to schedule packets among multiple CoS queues.
 * To cooperate with ECN, MLFQ also has a ECN marking threshold.
 * When the queue lengths of all CoS queues exceed this threshold, ECN is marked.
 * You can select enqueue marking or dequeue marking
 *
 * Variables:
 * queue_num_: number of CoS queues 
 * thresh_: ECN marking threshold (pkts)
 * dequeue_marking_: 1 (dequeue marking) or 0 (enqueue_marking) 
 * mean_pktsize_: configured mean packet size in bytes
 */
 
#ifndef ns_mlfq_h
#define ns_mlfq_h

#include <string.h>
#include "queue.h"
#include "config.h"

class MLFQ : public Queue {
	public:
		MLFQ() 
		{
			//Bind variables
			bind("queue_num_", &queue_num_);
			bind("thresh_",&thresh_);
			bind("dequeue_marking_",&dequeue_marking_);
			bind("mean_pktsize_", &mean_pktsize_);
			
			//Init queues
			q_=new PacketQueue*[queue_num_];
			for(int i=0;i<queue_num_;i++)
			{
				q_[i]=new PacketQueue;
			}
		}
		
		~MLFQ() 
		{
			for(int i=0;i<queue_num_;i++)
			{
				delete[] q_[i];
			}
			delete[] q_;
		}
	protected:
		void enque(Packet*);	// enqueue function
		Packet* deque();        // dequeue function
		
		int mean_pktsize_;		// configured mean packet size in bytes 
		
		PacketQueue **q_;		// underlying multi-FIFO (CoS) queues
		int thresh_;			// single ECN marking threshold
		int queue_num_;			// number of CoS queues
		int dequeue_marking_;	// 1 (dequeue marking) or 0 (enqueue_marking)
		
		//Return total queue length (pkts)
		int queueLength() 
		{
			int length = 0;
			for(int i=0; i<queue_num_; i++)
				length += q_[i]->length();
			return length;
		}
		
		//Return total queue length (bytes)
		int queueByteLength() 
		{
			int bytelength = 0;
			for(int i=0; i<queue_num_; i++)
				bytelength += q_[i]->byteLength();
			return bytelength;
		}
};

#endif
	

