#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <iostream>
using namespace std;

// 进程类型枚举，定义了两种进程类型：生产者和消费者
typedef enum { PRODUCER, CONSUMER } ProcessType;

// 进程控制块（PCB）结构体，用于存储进程的相关信息
typedef struct PCB {
    int pid;            // 进程ID，用于唯一标识一个进程
    ProcessType type;   // 进程类型，是生产者还是消费者
    char product;       // 生产/消费的产品，用字符表示
    bool t;             // 标识进程是否为唤醒的进程，默认为否 
    struct PCB *next;   // 队列指针，用于连接队列中的下一个进程控制块
} PCB;

// 队列结构，包含队列的头指针和尾指针
typedef struct {
    PCB *front;
    PCB *rear;
} Queue;

// 全局变量
Queue ready_queue;      // 就绪队列，存储准备运行的进程
Queue producer_queue;   // 生产者等待队列，存储因缓冲区满而等待的生产者进程
Queue consumer_queue;   // 消费者等待队列，存储因缓冲区空而等待的消费者进程
Queue over_list;        // 运行结束的进程链表，存储已经运行完毕的进程
char *buffer;           // 缓冲区指针，指向动态分配的缓冲区内存
int buffer_size;        // 缓冲区大小，由用户自定义
int buffer_count = 0;   // 当前缓冲区中产品数量，初始为0
int empty,full;			// 记录空缓冲区和满缓冲区的个数，作为同步信号量 
int mutex = 1;          // 模拟互斥信号量，1表示可用，用于控制对缓冲区的互斥访问
int next_pid = 1;       // 进程ID计数器，用于为新创建的进程分配唯一的ID
int N = 1;              // 记录进程调度顺序 

// 释放动态分配的内存并重置参数 
void reset(){
	printf("\n\n已清除缓冲区[ ");
    for (int i = 0; i < buffer_count; i++) printf("%c ", buffer[i]);  // 输出缓冲区状态
    printf("]内存\n");
	free(buffer);
	buffer_size = 0;
	buffer_count = 0;
	mutex = 1;
	next_pid = 1;
	N = 1; 
	PCB *p;
	printf("已清除就绪队列（");
	while(ready_queue.front != NULL){
		p = ready_queue.front;
		ready_queue.front = p -> next;
		printf("P%d ",p -> pid);
		free(p);
	}
	printf("）内存\n");
	printf("已清除生产者等待队列（");
	while(producer_queue.front != NULL){
		p = producer_queue.front;
		producer_queue.front = p -> next;
		printf("P%d ",p -> pid);
		free(p);
	}
	printf("）内存\n");
	printf("已清除消费者等待队列（");
	while(consumer_queue.front != NULL){
		p = consumer_queue.front;
		consumer_queue.front = p -> next;
		printf("P%d ",p -> pid);
		free(p);
	}
	printf("）内存\n");
	printf("已清除执行完的进程链表（");
	while(over_list.front != NULL){
		p = over_list.front;
		over_list.front = p -> next;
		printf("P%d ",p -> pid);
		free(p);
	}
	printf("）内存\n\n");
} 

// 初始化队列，将队列的头指针和尾指针都置为NULL
void init_queue(Queue *q) {
    q->front = q->rear = NULL;
}

// 入队操作，将一个进程控制块加入到队列的尾部（尾插法 
void enqueue(Queue *q, PCB *proc) {
    proc->next = NULL;  // 将新进程的下一个指针置为NULL
    if (q->rear == NULL) {  // 如果队列为空
        q->front = q->rear = proc;  // 则新进程既是队列的头也是尾
    } else {
        q->rear->next = proc;  // 将新进程连接到队列的尾部
        q->rear = proc;  // 更新队列的尾指针
    }
}

// 出队操作，从队列的头部移除一个进程控制块并返回
PCB* dequeue(Queue *q) {
    if (q->front == NULL) return NULL;  // 如果队列为空，返回NULL
    PCB *proc = q->front;  // 保存队列的头进程
    q->front = q->front->next;  // 更新队列的头指针
    if (q->front == NULL) q->rear = NULL;  // 如果队列变空，更新尾指针
    proc->next = NULL;  // 将出队进程的下一个指针置为NULL
    return proc;  // 返回出队的进程
}

// P操作（申请资源），模拟信号量的P操作
void P(int *semaphore, PCB *p) {
    (*semaphore)--;  // 信号量减1
    if (*semaphore < 0 && p != NULL) {  // 如果信号量小于0，说明资源不足
        if (p->type == PRODUCER) {  // 如果是生产者进程
            enqueue(&producer_queue, p);  // 将其加入生产者等待队列
        } 
		else if (p->type == CONSUMER) {
            enqueue(&consumer_queue, p);  // 将其加入消费者等待队列
        }
    }
}

// V操作（释放资源），模拟信号量的V操作
void V(int *semaphore, PCB *p) {
    (*semaphore)++;  // 信号量加1
    if (*semaphore <= 0 && p != NULL) {  // 如果信号量小于等于0，说明有进程在等待资源
        PCB *proc = NULL;
        if (producer_queue.front && p->type == CONSUMER) {  // 如果生产者等待队列中有进程
            proc = dequeue(&producer_queue);  // 从生产者等待队列中取出一个进程
            printf("     empty <= 0：唤醒生产者等待队列中阻塞的 P%d\n", proc->pid);  // 输出唤醒信息
        } else if (consumer_queue.front && p->type == PRODUCER) {  // 如果消费者等待队列中有进程
            proc = dequeue(&consumer_queue);  // 从消费者等待队列中取出一个进程
            printf("     full <= 0：唤醒消费者等待队列中阻塞的 P%d\n", proc->pid);  // 输出唤醒信息
        }
        if(proc){
        	proc->t = true;
        	enqueue(&ready_queue, proc);
		}
        printf("当前就绪队列列表:");
    	proc = ready_queue.front;
    	if(proc == NULL)printf("无");
   		while (proc != NULL) {
        	printf("P%d ", proc->pid);
        	proc = proc->next;
    	}
    	printf("\n");
        /*if (proc) { // 将唤醒的进程加入就绪队列（头插法 
			proc->next = ready_queue.front;
			ready_queue.front = proc;
			if(ready_queue.rear == NULL)ready_queue.rear = proc;
			PCB *current = dequeue(&ready_queue);  // 从就绪队列中取出一个进程
			if(proc->type == PRODUCER) {
				if (buffer_count < buffer_size) {  // 如果缓冲区还有空间
        			buffer[buffer_count++] = proc->product;  // 将产品放入缓冲区，并增加缓冲区计数
        			printf("P%d（生产者）生产了产品 %c ，缓冲区状态: [ ", proc->pid, proc->product);
        			for (int i = 0; i < buffer_count; i++) printf("%c ", buffer[i]);  // 输出缓冲区状态
        			printf("]\n");
        			enqueue(&over_list, proc);  // 将运行完毕的进程加入over链表
    			} 
			}
			else{
				if (buffer_count > 0) {  // 如果缓冲区中有产品
        			char product = buffer[--buffer_count];  // 从缓冲区取出一个产品，并减少缓冲区计数
        			printf("P%d（消费者）消费了产品 %c ，缓冲区状态: [ ", proc->pid, product);
        			for (int i = 0; i < buffer_count; i++) printf("%c ", buffer[i]);  // 输出缓冲区状态
        			printf("]\n");
        			enqueue(&over_list, proc);  // 将运行完毕的进程加入over链表
    			} 
			}
		}*/
    }
}

void show(void); // 函数声明 

// 生产者逻辑，实现生产者生产产品的操作
void produce(PCB *proc) {
	if(proc->t) goto J;
	P(&empty, proc); // 同步（放在互斥前防止阻塞死锁
	printf("执行%d:\tempty = %d",N++,empty);
	if(empty < 0){
		printf("\n    （P操作结束，empty < 0：睡眠，进程进入等待队列）\nP%d（生产者）因缓冲区满被阻塞，进入生产者等待队列：", proc->pid);  // 输出阻塞信息");
		PCB *p = producer_queue.front;
		if(p==NULL)printf("无");
    	while (p != NULL) {
        	printf("P%d ", p->pid);
        	p = p->next;
    	}
    	printf("\n");
	}else{
		J:
		if(proc->t) printf("执行%d:\t",N++);
		P(&mutex,NULL);  // 互斥：进入临界区（缓冲区），申请资源
		if(!proc->t) printf("，");
		printf("mutex = %d\n    （P操作结束，",mutex);
		printf("empty >= 0：进入临界区）\n");
		if (buffer_count >= buffer_size){
			enqueue(&producer_queue, proc);  // 将其加入生产者等待队列
			printf("但缓冲区已满，进入生产者等待队列：");
			PCB *p = producer_queue.front;
			if(p==NULL)printf("无");
    		while (p != NULL) {
        		printf("P%d ", p->pid);
        		p = p->next;
    		}
    		printf("\n");
		}
		if (buffer_count < buffer_size) {  // 如果缓冲区还有空间
        buffer[buffer_count++] = proc->product;  // 将产品放入缓冲区，并增加缓冲区计数
        printf("P%d（生产者）生产了产品 %c ，缓冲区状态: [ ", proc->pid, proc->product);
        for (int i = 0; i < buffer_count; i++) printf("%c ", buffer[i]);  // 输出缓冲区状态
        printf("]\n");
        enqueue(&over_list, proc);  // 将运行完毕的进程加入over链表
    	} 
    	V(&mutex,NULL);  // 退出临界区，释放资源
    	V(&full, proc);  // V操作不会引起阻塞，两个V操作的顺序可以调换 
    	printf("\tmutex = %d，full = %d\n    （V操作结束，",mutex,full);
    	printf("退出临界区，释放资源）\n");
	}
    show();
    printf("\n当前缓冲区状态: [ ");
    for (int i = 0; i < buffer_count; i++) printf("%c ", buffer[i]);  // 输出缓冲区状态
    printf("]\n");
    printf("各信号量的值：mutex = %d，empty = %d，full = %d\n\n",mutex,empty,full);
}

// 消费者逻辑，实现消费者消费产品的操作
void consume(PCB *proc) {
	if(proc->t) goto j;
	P(&full, proc);  // 同步（放在互斥前防止阻塞死锁
	printf("执行%d:\tfull = %d",N++,full);
	if(full < 0){
		printf("\n    （P操作结束，full < 0：睡眠，进程进入等待队列）\nP%d（消费者）因缓冲区空被阻塞，进入消费者等待队列：", proc->pid);  // 输出阻塞信息");
		PCB *p = consumer_queue.front;
		if(p==NULL)printf("无");
    	while (p != NULL) {
        	printf("P%d ", p->pid);
        	p = p->next;
    	}
    	printf("\n");
	}else{
		j:
		if(proc->t) printf("执行%d:\t",N++);
		P(&mutex,NULL);  // 互斥：进入临界区（缓冲区），申请资源
		if(!proc->t) printf("，");
		P(&mutex,NULL);  // 互斥：进入临界区（缓冲区），申请资源
		printf("mutex = %d\n    （P操作结束，",mutex);
		printf("full >= 0：进入临界区）\n");
		if (buffer_count == 0){
			enqueue(&consumer_queue, proc);  // 将其加入消费者等待队列
			printf("但缓冲区为空，进入消费者等待队列：");
			PCB *p = consumer_queue.front;
			if(p==NULL)printf("无");
    		while (p != NULL) {
        		printf("P%d ", p->pid);
        		p = p->next;
    		}
    		printf("\n");
		}
		if (buffer_count > 0) {  // 如果缓冲区中有产品
        	char product = buffer[--buffer_count];  // 从缓冲区取出一个产品，并减少缓冲区计数
        	printf("P%d（消费者）消费了产品 %c ，缓冲区状态: [ ", proc->pid, product);
        	for (int i = 0; i < buffer_count; i++) printf("%c ", buffer[i]);  // 输出缓冲区状态
        	printf("]\n");
        	enqueue(&over_list, proc);  // 将运行完毕的进程加入over链表
    	} 
    	V(&mutex,NULL);  // 退出临界区，释放资源
    	V(&empty, proc); // V操作不会引起阻塞，两个V操作的顺序可以调换
    	printf("\tmutex = %d，empty = %d\n    （V操作结束，",mutex,empty);
    	printf("退出临界区，释放资源）\n");
	}
    show();
    printf("\n当前缓冲区状态: [ ");
    for (int i = 0; i < buffer_count; i++) printf("%c ", buffer[i]);  // 输出缓冲区状态
    printf("]\n");
	printf("各信号量的值：mutex = %d，empty = %d，full = %d\n\n",mutex,empty,full);
}

// 调度器实现，负责调度就绪队列中的进程
void schedule() {
    while (ready_queue.front != NULL) {  // 只要就绪队列中有进程
        PCB *current = dequeue(&ready_queue);  // 从就绪队列中取出一个进程
        if (current->type == PRODUCER) {  // 如果是生产者进程
            produce(current);  // 执行生产操作
        } else {
            consume(current);  // 执行消费操作
        }
        printf("\n");
    }
}

// 打印over链表以及生产者与消费者等待中的进程
void show(){
	printf("\n就绪队列列表:");
    PCB *p = ready_queue.front;
    if(p==NULL)printf("无");
    while (p != NULL) {
        printf("P%d ", p->pid);
        p = p->next;
    }
	printf("\n已结束的进程列表:");
    p = over_list.front;
    if(p==NULL)printf("无"); 
    while (p != NULL) {
        printf("P%d ", p->pid);
        p = p->next;
    }
    printf("\n生产者等待列表:");
    p = producer_queue.front;
    if(p==NULL)printf("无");
    while (p != NULL) {
        printf("P%d ", p->pid);
        p = p->next;
    }
    printf("\n消费者等待列表:");
    p = consumer_queue.front;
    if(p==NULL)printf("无");
    while (p != NULL) {
        printf("P%d ", p->pid);
        p = p->next;
    }
}

int main() {
    // 用户自定义缓冲区大小
    Target:
    system("cls");
	printf("\n    -----------生产者和消费者问题-----------\n");
    printf("\n请输入缓冲区大小: ");
    while (scanf("%d", &buffer_size) != 1 || buffer_size <= 0) {  // 检查输入是否有效
        printf("输入无效，请重新输入正整数: ");
        while (getchar() != '\n'); // 清空输入缓冲区
    }
    while (getchar() != '\n');// 清空输入缓冲区
    buffer = (char*)malloc(buffer_size * sizeof(char)); // 动态分配缓冲区内存
    empty = buffer_size - buffer_count;
	full = buffer_count;
    // 初始化队列
    init_queue(&ready_queue);
    init_queue(&producer_queue);
    init_queue(&consumer_queue);
    init_queue(&over_list);
    char choice[3];
    do {
        // 用户输入进程类型和产品
        int type;
        char product = ' ';
        printf("\n输入待创建进程类型（0：生产者，1：消费者）: ");
        while (scanf("%d", &type) != 1 || (type != 0 && type != 1)) {  // 检查输入是否有效
            printf("输入无效，请重新输入（0：生产者，1：消费者）: ");
            while (getchar() != '\n');// 清空输入缓冲区
        }
		/*if (type == 0) {
        	while (getchar() != '\n');// 清空输入缓冲区
            printf("输入生产的产品字符: ");
            while (scanf(" %c", &product) != 1) { // 注意空格用于跳过换行符
                printf("输入无效，请重新输入字符: ");
                while (getchar() != '\n');
            }
        }*/
        if (type == 0) product = '*';
        while (getchar() != '\n');// 清空输入缓冲区
        // 创建进程并加入就绪队列
        PCB *proc = (PCB*)malloc(sizeof(PCB));
        proc->pid = next_pid++;
        proc->type = (type == 0) ? PRODUCER : CONSUMER;
        proc->product = product;
        proc->t = false;
        enqueue(&ready_queue, proc);
        printf("\n    成功创建 "); 
        printf("P%d（%s） ，", proc->pid, proc->type == PRODUCER ? "生产者" : "消费者");
        if(proc->type == PRODUCER)printf("生产产品 %c\n",proc->product);
        else printf("消费产品 %c\n",proc->product);
        printf("\n\n要继续添加下一个待创建的进程吗？( y表 是/其余字符表 否 ): ");
        cin >> choice;
        while (getchar() != '\n');// 清空输入缓冲区
    } while (choice[0] == 'y' || choice[0] == 'Y');
	//打印当前用户定义的就绪队列 
	PCB *p = ready_queue.front;
	system("cls");
	printf("\n目前初始就绪队列的进程：\n");
    while (p != NULL) {
        printf("P%d（%s） ，", p->pid, p->type == PRODUCER ? "生产者" : "消费者");
        if(p->type == PRODUCER)printf("生产产品 %c\n",p->product);
        else printf("消费产品 %c\n",p->product);
        p = p->next;
    }
    printf("\n初始状态：缓冲区大小为 %d，且mutex = %d，empty = %d，full = %d\n（当empty < 0时，其绝对值表示生产者等待队列里的进程数；当full < 0时，其绝对值表示消费者等待队列里的进程数）\n现在开始进行进程调度......\n\n",buffer_size,mutex,empty,full);
    schedule();  // 执行调度
    //show();    // 打印over链表以及生产者与消费者等待中的进程
    reset();    // 释放动态分配的内存并重置参数
    system("pause"); 
    printf("\n\n要继续创建进程调度吗？( y表 是/其余字符表 否 ): ");
    cin >> choice;
    while (getchar() != '\n');// 清空输入缓冲区
	if(choice[0] == 'y' || choice[0] == 'Y') goto Target;
	printf("\n\n已退出程序！");
    return 0;
}
