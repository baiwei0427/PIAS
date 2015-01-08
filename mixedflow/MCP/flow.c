#include "flow.h"
#include "params.h"

//Print a flow information
//Type: Add(0) Delete(1)
void MCP_Print_Flow(struct MCP_Flow* f, int type)
{		
	char local_ip[16]={0};           	//Local IP address 
	char remote_ip[16]={0};		    //Remote IP address 
	
	snprintf(local_ip, 16, "%pI4", &(f->local_ip));
	snprintf(remote_ip, 16, "%pI4", &(f->remote_ip));
	
	if(type==0)
	{
		printk(KERN_INFO "Insert a Flow record: %s:%hu to %s:%hu \n",local_ip,f->local_port,remote_ip,f->remote_port);
	}
	else if(type==1)
	{
		printk(KERN_INFO "Delete a Flow record: %s:%hu to %s:%hu \n",local_ip,f->local_port,remote_ip,f->remote_port);
	}
	else
	{
		printk(KERN_INFO "Flow record: %s:%hu to %s:%hu \n",local_ip,f->local_port,remote_ip,f->remote_port);
	}
}

//Print a FlowNode
void MCP_Print_Node(struct MCP_Flow_Node* fn)
{		
	MCP_Print_Flow(&(fn->f),2);
}

//Print a FlowList
void MCP_Print_List(struct MCP_Flow_List* fl);
{		
	struct MCP_Flow_Node* Ptr;
	for(Ptr=fl->head->next;Ptr!=NULL;Ptr=Ptr->next)
	{
		MCP_Print_Node(Ptr);
	}
}

//Print a FlowTable
void MCP_Print_Table(struct MCP_Flow_Table* ft)
{		
	int i=0;
	printk(KERN_INFO "Current flow table:\n");
	for(i=0;i<MCP_HASH_RANGE;i++)
    {
		if(ft->table[i].len>0)
        {
			printk(KERN_INFO "FlowList %d\n",i);
			Print_List(&(ft->table[i]));
        }
    }
	printk(KERN_INFO "There are %u flows in total\n",ft->size);
}

//Hash function, calculate the flow should be inserted into which MCP_Flow_List
unsigned int MCP_Hash(struct MCP_Flow* f)
{		
	//return a value in [0,HASH_RANGE-1]
	return ((f->local_ip/(256*256*256)+1)*
	(f->remote_ip/(256*256*256)+1)*
	(f->local_port+1)*
	(f->remote_port+1))%MCP_HASH_RANGE;
}

//Determine whether two Flows are equal 
//<local_ip, remote_ip, local_port, remote_port> determines a flow
unsigned int MCP_Equal(struct Flow* f1,struct Flow* f2)
{
	return ((f1->local_ip==f2->local_ip)
	&&(f1->remote_ip==f2->remote_ip)
	&&(f1->local_port==f2->local_port)
	&&(f1->remote_port==f2->remote_port));		
}

//Initialize the info of a Flow
void MCP_Init_Info(struct MCP_Flow_Info* info)
{
	info->latest_seq=0;
	info->bytes_received=0;
	info->bytes_total=0;
	info->window=0;
	info->srtt=0;
	info->bytes_rtt_received=0;
	info->bytes_rtt_received_ecn=0;
	info->deadline=ktime_set(0,0);
	info->last_update=ktime_set(0,0);
}

//Initialize a Flow
void MCP_Init_Flow(struct MCP_Flow* f)
{
	f->local_ip=0;	
	f->remote_ip=0;
	f->local_port=0;
	f->remote_port=0;
	
	//Initialize the Info of this Flow
	MCP_Init_Info(&(f->info));
}

//Initialize a FlowNode
void MCP_Init_Node(struct MCP_Flow_Node* fn)
{
	//Initialize next pointer as null
	fn->next=NULL;
	//Initialize a flow structure
	MCP_Init_Flow(&(fn->f));
}

//Initialize a FlowList
void MCP_Init_List(struct MCP_Flow_List* fl)
{
	struct MCP_Flow_Node* buf=NULL;
	//No node in current list
	fl->len=0;
	//We use vmalloc here since this function is ususally referenced when we inset the module
	buf=vmalloc(sizeof(struct  MCP_Flow_Node));
	if(buf==NULL)
	{
		printk(KERN_INFO "Vmalloc error in MCP_Init_List\n");
	}
	else
	{
		fl->head=buf;
		MCP_Init_Node(fl->head);
	}
}

//Initialize a FlowTable
void MCP_Init_Table(struct MCP_Flow_Table* ft)
{
	int i=0;
	struct MCP_Flow_List* buf=NULL;
	
	//allocate space for FlowLists
	buf=vmalloc(MCP_HASH_RANGE*sizeof(struct MCP_Flow_List));
	if(buf==NULL)
	{
		printk(KERN_INFO "Vmalloc error in MCP_Init_Table\n");
	}
	else
	{
		ft->table=buf;
		//Initialize each FlowList
		for(i=0;i<MCP_HASH_RANGE;i++)
		{
			MCP_Init_List(&(ft->table[i]));
		}
	}
	//No nodes in current table
	ft->size=0;
}

//Insert a Flow into a FlowList and return 1 if it succeeds
unsigned int MCP_Insert_List(struct MCP_Flow_List* fl, struct MCP_Flow* f, int flags)
{
	if(fl->len>=MCP_LIST_SIZE) 
	{
		printk(KERN_INFO "MCP_Insert_List: No enough space in this link list\n");
		return 0;
	} 
	else 
	{
        struct MCP_Flow_Node* tmp=fl->head;
		struct MCP_Flow_Node* buf=NULL;

        //Come to the tail of this FlowList
        while(1)
        {
			//If pointer to next node is NULL, we find the tail of this FlowList. Here we can insert our new Flow
            if(tmp->next==NULL)
            {
				//Allocate memory. The 'flags' is determined by context
				buf=kmalloc(sizeof(struct MCP_Flow_Node),flags);
				if(buf==NULL) //Fail to allocate memory
				{
					printk(KERN_INFO "MCP_Insert_List: Kmalloc error\n");
					return 0;
				}
				else
				{
					tmp->next=buf;
					//Copy data for this new FlowNode
					tmp->next->f=*f;
					//Pointer to next FlowNode is NUll
					tmp->next->next=NULL;
					//Increase length of FlowList
					fl->len++;
					return 1;
				}
			}
            //If the rule of next node is the same as our inserted flow, we just finish the insert  
			else if(MCP_Equal(&(tmp->next->f),f)==1) 
			{
				printk(KERN_INFO "MCP_Insert_List: Equal Flow\n");
				return 0;
			}
            else //Move to next FlowNode
            {
				tmp=tmp->next;
            }
       }
		return 0;
	}
}

//Insert a flow to FlowTable and return 1 if it succeeds
unsigned int MCP_Insert_Table(struct MCP_Flow_Table* ft,struct MCP_Flow* f, int flags)
{
	unsigned int result=0;
	unsigned int index=Hash(f);
	
	//printk(KERN_INFO "Insert to link list %d\n",index);
	//Insert Flow to appropriate FlowList based on Hash value
	result=MCP_Insert_List(&(ft->table[index]),f,flags);
	//Increase the size of FlowTable
	ft->size+=result;
	
	return result;
}

//Search and return the pointer of information for a given flow in a FlowList
struct MCP_Flow_Info* MCP_Search_List(struct MCP_Flow_List* fl, struct MCP_Flow* f)
{
	//The length of FlowList is 0
	if(fl->len==0) 
	{
		return NULL;
	} 
	else 
	{
		struct MCP_Flow_Node* tmp=fl->head;
		//Find the Flow in this FlowList
		while(1)
		{
			//If pointer to next node is NULL, we find the tail of this FlowList, no more FlowNodes to search
			if(tmp->next==NULL)
			{
				return NULL;
			}
			//Find matching flow (matching FlowNode is tmp->next rather than tmp)
			else if(MCP_Equal(&(tmp->next->f),f)==1)
			{
				//return the info of this Flow
				return &(tmp->next->f.info);
			}	
			else
			{
				//Move to next FlowNode
				tmp=tmp->next;
			}
		}
	}
	return NULL;
}

//Search the information for a given Flow in a FlowTable
struct MCP_Flow_Info*MCP_ Search_Table(struct MCP_Flow_Table* ft, struct MCP_Flow* f)
{
	unsigned int index=0;
	index=Hash(f);
	return MCP_Search_List(&(ft->table[index]),f);
}



