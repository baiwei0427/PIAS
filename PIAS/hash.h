#ifndef HASH_H
#define HASH_H

#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/vmalloc.h>

#include "flow.h" //Get definition of Flow structure

#define HASH_RANGE 1024
#define QUEUE_SIZE 128

//Node of Flow
struct FlowNode{
        struct Flow f;         				//content of flow
        struct FlowNode* next; 	//pointer to next node 
};

//List of Flows
struct FlowList{
        struct FlowNode* head; 	//pointer to head node of this link list
        int len;              						 //current length of this list (max: QUEUE_SIZE)
};

//Hash Table of Flows
struct FlowTable{
        struct FlowList* table; 		//many FlowList (HASH_RANGE)
        int size;              					 //total number of nodes in this table
};

//Hash function, given a Flow node, calculate it should be inserted into which FlowList
static unsigned int Hash(struct Flow* f)
{
	//return a value in [0,HASH_RANGE-1]
	return ((f->local_ip/(256*256*256)+1)*(f->remote_ip/(256*256*256)+1)*(f->local_port/(256*256*256)+1)*(f->remote_port/(256*256*256)+1))%HASH_RANGE;
}

//Determine whether two Flows are equal (same flow) 
static int Equal(struct Flow* f1,struct Flow* f2)
{
	//<local_ip,local_port,remote_ip,remote_port> determines a TCP flow
	if((f1->local_ip==f2->local_ip)&&(f1->remote_ip==f2->remote_ip)&&(f1->local_port==f2->local_port)&&(f1->remote_port==f2->remote_port))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

//Initialize a TCP flow information entry
static void Init_Information(struct Information* info)
{
	info->start_time=0;
	info->latest_receive_time=0;
	info->srtt=0;
	info->seq=0;
	info->ack=0;
	info->receive_data=0;
	info->send_data=0;
	info->status=0;
}

//Initialize a complete TCP flow 
static void Init_Flow(struct Flow* f)
{
	f->direction=0;
	f>remote_ip=0;
	f->local_ip=0;
	f->remote_port=0;
	f->local_port=0;
	Init_Information(&(f->info));
}

//Initialize a FlowNode
static void Init_Node(struct FlowNode* fn)
{
	//Initialize the pointer to next node as NULL
	fn->next=NULL;
	Init_Flow(&(fn->f));
}

//Initialize a FlowList
static void Init_List(struct FlowList* fl)
{
	//No node in current list
	fl->len=0;
	fl->head=vmalloc(sizeof(struct  FlowNode));
	Init_Node(fl->head);
} 

//Initialize FlowTable
static void Init_Table(struct FlowTable* ft)
{
	int i=0;
	//Allocate space for FlowLists
	ft->table=vmalloc(HASH_RANGE*sizeof(struct FlowList));
	//Initialize each FlowList
	for(i=0;i<HASH_RANGE;i++)
	{
		Init_List(&(ft->table[i]));
	}
	//No nodes in current table
	ft->size=0;
}

//Insert a new Flow entry into a FlowList
//If the Flow entry is inserted successfully, return 1
//Else, fl->len>=QUEUE_SIZE or the same Flow entry exists, return 0
static int Insert_List(struct FlowList* fl, struct Flow* f)
{
	//Reach maximum queue size in this list
	if(fl->len>=QUEUE_SIZE)
	{
		//printk(KERN_INFO "No enough space in this FlowList\n");
		return 0;
	}
	else
	{
		struct FlowNode* tmp=fl->head;
		
		//Find the tail of FlowList
		while(1)
		{
			//If pointer to next node is NULL, we find the tail of this FlowList. Here we can insert our new Flow entry
			if(tmp->next==NULL)
			{
				//Allocate space for next FlowNode
				tmp->next=vmalloc(sizeof(struct FlowNode));
				//Copy data for this new FlowNode
				tmp->next->f=*f;
				//Pointer to next node is NULL
				tmp->next->next=NULL;
				//Increase the length of current FlowList
				fl->len++;
				//Finish the insert, return 1
				return 1;
			}
			//If we find the same Flow entry, we sould return 0 
			else if(Equal(&(tmp->next->f),f)==1)
			{
				return 0;
			}
			//Move to next FlowNode
			else
			{
				tmp=tmp->next;
			}
		}
	}
	//By default, return 0
	return 0;
}

//Insert a new Flow entry into a FlowTable
//If success, return 1. Else return 0
static void Insert_Table(struct FlowTable* ft,struct Flow* f)
{
		int result=0;
        unsigned int index=Hash(f);
       // printk(KERN_INFO "Insert to FlowList:%u\n",index);
        //Insert Flow to appropriate FlowList based on Hash value
        result=Insert_List(&(ft->table[index]),f);
        //Increase the size of FlowTable
        rt->size+=result;
        //printk(KERN_INFO "Insert complete\n");
		return result;
}

//Search the information for a given TCP flow in a FlowList
//Note: we return the pointer to the structure of Information so that we can update the information of this flow
static struct Information* Search_List(struct FlowList* fl, struct Flow* f)
{
	//If the length of this FlowList is 0, return NULL
	if(fl->len==0)
	{
		return NULL;
	}
	else
	{
		//Get head node of this list
		struct FlowNode* tmp=fl->head;
		
		//Find the flow entry in this list
		while(1)
		{
			//If the pointer to next node is NULL, we don't find matching flow entry and return NULL
			if(tmp->next==NULL)
			{
				return NULL;
			}
			//Find matching flow entry 
			else if(Equal(&(tmp->next->f),f)==1)
			{
				//return the pointer to information of this FlowNode
				return &(tmp->next->f.info);
			}
			else
			{
				//Move to next FlowNode
				tmp=tmp->next;
			}
		}
	}
	//By default, return NULL
	return NULL;
}

//Search the information for a given Flow in a FlowTable
static struct Infomation* Search_Table(struct FlowTable* ft, struct Flow* f)
{
	unsigned int index=0;
	index=Hash(f);
	return Search_List(&(ft->table[index]),f);
}

//Delete a given Flow entry from FlowList
//If the Flow entry is successfully deleted, return 1
//Else, return 0
static int Delete_List(struct FlowList* fl, struct Flow* f)
{
	//No node in current FlowList
	if(fl->len==0) 
	{
		//printk(KERN_INFO "No node in current list\n");
		return 0;
	}
	else 
	{
		//Get the head node of current FlowList
		struct FlowNode* tmp=fl->head;
		while(1)	
		{
			//If pointer to next node is NULL, we come to the tail of this FlowList, return 0
			if(tmp->next==NULL) 
			{
				//printk(KERN_INFO "There are %d flows in this list\n",fl->len);
				return 0;
			}
			//Find the matching FlowNode (matching FlowNode is tmp->next rather than tmp, we should delete tmp->next)
			else if(Equal(&(tmp->next->f),f)==1) 
			{
				struct FlowNode* s=tmp->next;
				
				tmp->next=s->next;
				//Delete matching FlowNode from this FlowList
				vfree(s);
				//Reduce the length of this FlowList by one
				fl->len--;
				//printk(KERN_INFO "Delete a flow record\n");
				return 1;
			}
			else //No matching FlowNode
			{
				//Move to next FlowNode
				tmp=tmp->next;
			}
		}
	}
	return 0;
}

//Delete a given Flow entry from FlowTable
//If success, return 1. Else, return 0
static int Delete_Table(struct FlowTable* ft,struct Flow* f)
{
	int result=0;
	unsigned int index=0;
	index=Hash(f);
	//Delete Flow from appropriate FlowList based on Hash value
	result=Delete_List(&(ft->table[index]),f);
	//Reduce the size of FlowTable according to return result of Delete_List
	ft->size-=result;
	//printk(KERN_INFO "Delete %d \n",result);
	return result;
}

//Clear a FlowList
static void Empty_List(struct FlowList* fl)
{
	struct FlowNode* NextNode;
	struct FlowNode* Ptr;
	for(Ptr=fl->head;Ptr!=NULL;Ptr=NextNode)
	{
		NextNode=Ptr->next;
		//Actually, we delete the fl->head in the first iteration
		vfree(Ptr);
	}
}

//Clear a FlowTable
static void Empty_Table(struct FlowTable* ft)
{
	int i=0;
	for(i=0;i<HASH_RANGE;i++)
	{
		Empty_List(&(ft->table[i]));
	}
	vfree(ft->table);
}

//Print a flow information
//Type: Insert(0) Delete(1)
static void Print_Flow(struct Flow* f, int type)
{
	char local_ip[16]={0};           
	char remote_ip[16]={0};           
	
	snprintf(local_ip, 16, "%pI4", &(f->local_ip));
	snprintf(remote_ip, 16, "%pI4", &(f->remote_ip));
	
	if(type==0) //Insert
	{
		printk(KERN_INFO "Insert a Flow record <%s:%hu , %s:%hu> \n",local_ip,f->local_port,remote_ip,f->remote_port);
	}
	else if(type==1) //Delete
	{
		printk(KERN_INFO "Delete a Flow record <%s:%hu , %s:%hu >\n",local_ip,f->local_port,remote_ip,f->remote_port);
	}
	else //Otherwise
	{
		printk(KERN_INFO "Flow record <%s:%hu , %s:%hu > \n",local_ip,f->local_port,remote_ip,f->remote_port);
	}
}

//Print a FlowNode
static void Print_Node(struct FlowNode* fn)
{
	Print_Flow(&(fn->f),2);
}

//Print a FlowList
static void Print_List(struct FlowList* fl)
{
	struct FlowNode* Ptr;
	for(Ptr=fl->head->next;Ptr!=NULL;Ptr=Ptr->next)
	{
		if(Ptr!=NULL)
		{
			Print_Node(Ptr);
		}
	}
}

//Print a FlowTable
static void Print_Table(struct FlowTable* ft)
{
	int i=0;
	printk(KERN_INFO "Current flow table:\n");
	for(i=0;i<HASH_RANGE;i++)
	{
		if(ft->table[i].len>0)
		{
			//printk(KERN_INFO "FlowList %d\n",i);
			Print_List(&(ft->table[i]));          
        }
    }
	printk(KERN_INFO "There are %d flows in total\n",ft->size);
}


#endif 












