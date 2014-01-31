/*******************************************************
	    HEFT DUPLICATION IMPLEMENTATION
********************************************************/
//NOTES
//UPPER RANK ARRAY(tasks_upper_rank[]) INITIALIZED TO -1 SO AS TO PREVENT REDUNDANT CALCULATION OF UPPER RANKS
//COMMUNICATION COST MATRIX(data[][]) HAS -1 VALUE FOR NON COMMUNICATING TASKS


#include<stdio.h>
#include<stdlib.h>
int no_tasks,no_machines;
double **computation_costs,**data_transfer_rate,**data,*tasks_upper_rank;
int *sorted_tasks;

struct TaskProcessor
{
    int processor;
    double AST;
    double AFT;
};

struct TaskProcessor *schedule;
/*******************************************************
	FUNCTION FOR DETERMINATION OF UPPER RANK
********************************************************/
// Calculate average communication cost and give feed to sorted task array
void insertinto(int task,double rank)
{
    static int pos;
    int i;
    for(i=pos-1; i>=0; i--)
        if(rank>tasks_upper_rank[sorted_tasks[i]])
            sorted_tasks[i+1]=sorted_tasks[i];
        else
            break;
    sorted_tasks[i+1]=task;
    pos++;
}

// Calculate the average cost of communication between source and destination
double avg_communicationcost(int source,int destination)
{
    int i,j;
    double avg=0.0;
    for(i=0; i<no_machines; i++)
        for(j=0; j<no_machines; j++)
        {
            if(data_transfer_rate[i][j]!=0)
                avg+=(data[source][destination]/data_transfer_rate[i][j]);
        }
    avg=avg/(no_machines*no_machines-no_machines);
    return avg;
}

// Calculate the upper rank
double calculate_upper_rank(int task)
{
    int j;
    double avg_communication_cost,successor,avg=0.0,max=0,rank_successor;

    for(j=0; j<no_machines; j++)
        avg+=computation_costs[task][j];
    avg/=no_machines;

    for(j=0; j<no_tasks; j++)
    {
        // Check if node(j) is a successor of node(task)
        if(data[task][j]!=-1)
        {
            avg_communication_cost=avg_communicationcost(task,j);
            if(tasks_upper_rank[j]==-1)
            {
                rank_successor= tasks_upper_rank[j]= calculate_upper_rank(j);
                insertinto(j,rank_successor);
            }
            else
                rank_successor= tasks_upper_rank[j];

            successor=avg_communication_cost+rank_successor;

            if(max<successor)
                max=successor;
        }
    }
    return(avg+max);
}

/*******************************************************
	FUNCTION FOR DETERMINATION OF SCHEDULE
********************************************************/
void insertslots(double **machineFreeTime,int current_pos, double start,double end)
{
    int i;
    //printf("%lf %lf\n",start,end);
    if(start < 0)
        start=0;
    for(i=current_pos-1; i>=0; i--)
    {
        if(start < machineFreeTime[i][0])
        {
            machineFreeTime[i+1][0]=machineFreeTime[i][0];
            machineFreeTime[i+1][1]=machineFreeTime[i][1];
        }
        else
            break;
    }
    machineFreeTime[i+1][0]=start;
    machineFreeTime[i+1][1]=end;
}
void findfreeslots(int processor,double **machineFreeTime,int *noslots)
{
    int i,j;
    *noslots=0;
    double highest_AFT=-99999.0,min=99999.0;
    for(i=0; i<no_tasks; i++)
    {
        min=99999.0;
        if(schedule[i].processor==processor)
        {
            if(schedule[i].AFT>highest_AFT)
                highest_AFT=schedule[i].AFT;
            for(j=0; j<no_tasks; j++)
            {
                if((i==j) || (schedule[j].processor!=processor))
                    continue;
                if((schedule[j].AST>=schedule[i].AFT) && (schedule[j].AST<min))
                {
                    min=schedule[j].AST;
                }
            }
            if(min<99998.0)
            {
                insertslots(machineFreeTime,*noslots,schedule[i].AFT,min);
                (*noslots)++;
            }
        }
    }
    insertslots(machineFreeTime,*noslots,highest_AFT,99999.0);
    (*noslots)++;
}

// Ckeck if it is an entry task
int isEntryTask(int task)
{
    int i;
    for(i=0; i<no_tasks; i++)
    {
        if(data[i][task]!=-1)
            return 0;
    }
    return 1;
}

// Find EST
double find_EST(int task,int processor)
{
    int i;
    double ST,EST=-99999.0,comm_cost;
    for(i=0; i<no_tasks; i++)
    {
        if(data[i][task]!=-1)
        {
            // If they use the same processor, the cost will be 0
            if(data_transfer_rate[schedule[i].processor][processor]==0)
                comm_cost=0;
            // Otherwise
            else
                comm_cost=data[i][task]/data_transfer_rate[schedule[i].processor][processor];
            ST=schedule[i].AFT + comm_cost;
            // Try to find the max EST
            if(EST<ST)
                EST=ST;
        }
    }
    return EST;
}

// Calculate the EST and EFT
void calculate_EST_EFT(int task,int processor,struct TaskProcessor *EST_EFT)
{
    double **machineFreeTime,EST;
    int i;
    machineFreeTime=(double**)calloc(100,sizeof(double*));
    for(i=0; i<100; i++)
        machineFreeTime[i]=(double*)calloc(2,sizeof(double));
    int noslots=0;
    findfreeslots(processor,machineFreeTime,&noslots);
    //if(task==2)
    //for(i=0;i<noslots;i++)
    //{
    //	printf("%lf %lf\n",machineFreeTime[i][0],machineFreeTime[i][1]);
    //}
    printf("\n\n");
    EST=find_EST(task,processor);
    //printf("%lf\n",EST);
    EST_EFT->AST=EST;
    EST_EFT->processor=processor;
    for(i=0; i<noslots; i++)
    {
        if(EST<machineFreeTime[i][0])
        {
            if((machineFreeTime[i][0] + computation_costs[task][processor]) <= machineFreeTime[i][1])
            {
                EST_EFT->AST=machineFreeTime[i][0];
                EST_EFT->AFT=machineFreeTime[i][0] + computation_costs[task][processor];
                return;
            }
        }
        if(EST>=machineFreeTime[i][0])
        {
            if((EST + computation_costs[task][processor]) <= machineFreeTime[i][1])
            {
                EST_EFT->AFT=EST_EFT->AST + computation_costs[task][processor];
                return;
            }
        }
    }
}

void make_schedule()
{
    int i,j,k,t=0,processor,task;
    double minCost=99999.99,min_EFT=99999.99;
    struct TaskProcessor *EST_EFT;
    EST_EFT=(struct TaskProcessor *)calloc(1,sizeof(struct TaskProcessor));

    for(i=0; i<no_tasks; i++)
    {
        min_EFT=9999.99;
        task=sorted_tasks[i];
        // Check if it is a start task
        if(isEntryTask(task))
        {
            for(j=0; j<no_machines; j++)
            {
                if(minCost>computation_costs[task][j])
                {
                    minCost=computation_costs[task][j];
                    processor=j;
                }
            }
            schedule[task].processor=processor;
            schedule[task].AST=0;
            schedule[task].AFT=minCost;
        }
        else
        {
            for(j=0; j<no_machines; j++)
            {
                calculate_EST_EFT(task,j,EST_EFT);
                printf("%lf %lf %d",EST_EFT->AST,EST_EFT->AFT,EST_EFT->processor);
                if(min_EFT>(EST_EFT->AFT))
                {
                    schedule[task]=*EST_EFT;
                    min_EFT=EST_EFT->AFT;
                }
            }
        }

        printf("\nTask scheduled %d\n",task);
        printf("%d %lf %lf\n",schedule[task].processor,schedule[task].AST,schedule[task].AFT);
        printf("------\n");
    }
}

/*******************************************************
	FUNCTION FOR DISPLAYING SCHEDULE
********************************************************/
void display_schedule()
{
    int i;
    printf("\n\nSCHEDULE\n\n");
    for(i=0; i<no_tasks; i++)
    {
        printf("TASK :%d PROCESSOR :%d AST :%lf AFT :%lf\n",i+1,schedule[i].processor+1,schedule[i].AST,schedule[i].AFT);
    }
}


int main()
{
    int i,j;
    FILE *fp;
    fp=fopen("Input.txt","r+");

    // Read number of tasks and number of processors
    fscanf(fp,"%d%d",&no_tasks,&no_machines);

    // Initialize Computation Cost Table
    computation_costs=(double**)calloc(no_tasks,sizeof(double*));
    // Initialize each row
    for(i=0; i<no_tasks; i++)
        computation_costs[i]=(double*)calloc(no_machines,sizeof(double));

    // Initialize Data Transfer Rate Table
    data_transfer_rate=(double**)calloc(no_machines,sizeof(double*));
    // Initialize each row
    for(i=0; i<no_machines; i++)
        data_transfer_rate[i]=(double*)calloc(no_machines,sizeof(double));

    // Initialize Data Table
    data=(double**)calloc(no_tasks,sizeof(double*));
    // Initialize each row
    for(i=0; i<no_tasks; i++)
        data[i]=(double*)calloc(no_tasks,sizeof(double));

    // Initialize Tasks Upper Rank Table
    tasks_upper_rank=(double *)calloc(no_tasks,sizeof(double));
    // Initialize each row
    for(i=0; i<no_tasks; i++)
        tasks_upper_rank[i]=-1;

    sorted_tasks=(int *)calloc(no_tasks,sizeof(int));
    schedule=(struct TaskProcessor*)calloc(no_tasks,sizeof(struct TaskProcessor));
    for(i=0; i<no_tasks; i++)
        schedule[i].processor=-1;

    // Read the computation costs of each task
    for(i=0; i<no_tasks; i++)
        for(j=0; j<no_machines; j++)
            fscanf(fp,"%lf",&computation_costs[i][j]);
    // Read the matrix of data transfer rate between two processors
    for(i=0; i<no_machines; i++)
        for(j=0; j<no_machines; j++)
            fscanf(fp,"%lf",&data_transfer_rate[i][j]);
    // Read the matrix of data to be transferred between two tasks
    for(i=0; i<no_tasks; i++)
        for(j=0; j<no_tasks; j++)
            fscanf(fp,"%lf",&data[i][j]);

    // Calculate upper rank
    for(i=0; i<no_tasks; i++)
    {
        if(tasks_upper_rank[i]==-1)
        {
            tasks_upper_rank[i]=calculate_upper_rank(i);
            insertinto(i,tasks_upper_rank[i]);
        }
    }

    printf("UPPER RANKS OF TASKS :\n\n");
    for(i=0; i<no_tasks; i++)
        printf("TASK NO. %d : %.2lf\n",i,tasks_upper_rank[i]);
    for(i=0; i<no_tasks; i++)
        printf("TASK NO. : %d\n",sorted_tasks[i]);

    make_schedule();

    display_schedule();

    scanf("%d",i);

    return 0;
}
