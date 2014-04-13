/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1994 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/queue/double-queue.cc,v 1.17 2014/04/10 23:35:37 haldar Exp $ (LBL)";
#endif

#include "double-queue.h"
#include "flags.h"

static class DoubleQueueClass : public TclClass {
 public:
	DoubleQueueClass() : TclClass("Queue/DoubleQueue") {}
	TclObject* create(int, const char*const*) {
		return (new DoubleQueue);
	}
} class_double_queue;

void DoubleQueue::reset()
{
	Queue::reset();
}

int 
DoubleQueue::command(int argc, const char*const* argv) 
{
	if (argc==2) {
		if (strcmp(argv[1], "printstats") == 0) {
			print_summarystats();
			return (TCL_OK);
		}
 		if (strcmp(argv[1], "shrink-queue") == 0) {
 			shrink_queue();
 			return (TCL_OK);
 		}
	}
	if (argc == 3) {
		if (!strcmp(argv[1], "packetqueue-attach")) {
			delete[] q_;
			bool result = false;
			int pos = 0;
			for(int i=0;i<QUE_NUM; i++) {
				result = (q_[i] = (PacketQueue*) TclObject::lookup(argv[2]));
				pos = i;
				if(result)
					break;
			}
			if(!result)
				return (TCL_ERROR);
			else {
				printf("hello\n");
				pq_ = q_[pos];
				return (TCL_OK);
			}
		}
	}
	return Queue::command(argc, argv);
}

/*
 * drop-tail
 */
void DoubleQueue::enque(Packet* p)
{
	//printf("Enque %d %d \n",q_[0]->length(),q_[1]->length());
	hdr_ip *iph = hdr_ip::access(p);
	int prio = iph->prio();

	hdr_flags* hf = hdr_flags::access(p);

	int qlimBytes = thresh_[prio] * mean_pktsize_;
	//printf("lim %d\n",qlimBytes);
	
	if((!qib_ && q_[prio]->length()+1 > thresh_[prio]) || (qib_ && q_[prio]->byteLength() > qlimBytes )) {
		//printf("incast happen\n");
		//printf("qlim_ %d\n",qlim_);
		if(qlim_-queueLength() > 0){
			if (hf->ect()) {
				hf->ce() = 1;
				//printf("mark ecn\n");
				q_[prio]->enque(p);
			} else {
				q_[prio]->enque(p);
			}
		}
		else {
			//printf("qlim_ %d, qLength %d\n",qlim_,queueLength());
			drop(p);
		}
	}
	else {
		q_[prio]->enque(p);
		//printf("queue[0] %d\n", q_[0]->length());
	}
	//printf("Enque %d %d \n",q_[0]->length(),q_[1]->length());
}

//AG if queue size changes, we drop excessive packets...
void DoubleQueue::shrink_queue() 
{
	int qlimBytes = qlim_ * mean_pktsize_;
	printf("fuck!!!!\n");
	if (debug_)
		printf("shrink-queue: time %5.2f qlen %d, qlim %d\n",
 			Scheduler::instance().clock(),
			queueLength(), qlim_);
	for(int prio = 0; prio<QUE_NUM; prio++){
		while ((!qib_ && q_[prio]->length() > qlim_) || 
			(qib_ && q_[prio]->byteLength() > qlimBytes)) {
				if (drop_front_) { /* remove from head of queue */
						Packet *pp = q_[prio]->deque();
						drop(pp);
				} else {
						Packet *pp = q_[prio]->tail();
						q_[prio]->remove(pp);
						drop(pp);
				}
		}
	}
}

Packet* DoubleQueue::deque()
{
	//printf("Deque %d %d \n",q_[0]->length(),q_[1]->length());
	if (q_[0]->length() > 0){
		//printf("%s,%s,%d,prio %d\n",__FILE__,__func__,__LINE__,0);
		return q_[0]->deque();
	}
	else if(q_[1]->length() > 0){
		//printf("%s,%s,%d,prio %d\n",__FILE__,__func__,__LINE__,1);
		return q_[1]->deque();
	}
	return q_[0]->deque();
}

void DoubleQueue::print_summarystats()
{
	//double now = Scheduler::instance().clock();
        printf("True average queue: %5.3f", true_ave_);
        if (qib_)
                printf(" (in bytes)");
        printf(" time: %5.3f\n", total_time_);
}
