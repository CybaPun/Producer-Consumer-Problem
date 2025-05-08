#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <iostream>
using namespace std;

// ��������ö�٣����������ֽ������ͣ������ߺ�������
typedef enum { PRODUCER, CONSUMER } ProcessType;

// ���̿��ƿ飨PCB���ṹ�壬���ڴ洢���̵������Ϣ
typedef struct PCB {
    int pid;            // ����ID������Ψһ��ʶһ������
    ProcessType type;   // �������ͣ��������߻���������
    char product;       // ����/���ѵĲ�Ʒ�����ַ���ʾ
    bool t;             // ��ʶ�����Ƿ�Ϊ���ѵĽ��̣�Ĭ��Ϊ�� 
    struct PCB *next;   // ����ָ�룬�������Ӷ����е���һ�����̿��ƿ�
} PCB;

// ���нṹ���������е�ͷָ���βָ��
typedef struct {
    PCB *front;
    PCB *rear;
} Queue;

// ȫ�ֱ���
Queue ready_queue;      // �������У��洢׼�����еĽ���
Queue producer_queue;   // �����ߵȴ����У��洢�򻺳��������ȴ��������߽���
Queue consumer_queue;   // �����ߵȴ����У��洢�򻺳����ն��ȴ��������߽���
Queue over_list;        // ���н����Ľ��������洢�Ѿ�������ϵĽ���
char *buffer;           // ������ָ�룬ָ��̬����Ļ������ڴ�
int buffer_size;        // ��������С�����û��Զ���
int buffer_count = 0;   // ��ǰ�������в�Ʒ��������ʼΪ0
int empty,full;			// ��¼�ջ����������������ĸ�������Ϊͬ���ź��� 
int mutex = 1;          // ģ�⻥���ź�����1��ʾ���ã����ڿ��ƶԻ������Ļ������
int next_pid = 1;       // ����ID������������Ϊ�´����Ľ��̷���Ψһ��ID
int N = 1;              // ��¼���̵���˳�� 

// �ͷŶ�̬������ڴ沢���ò��� 
void reset(){
	printf("\n\n�����������[ ");
    for (int i = 0; i < buffer_count; i++) printf("%c ", buffer[i]);  // ���������״̬
    printf("]�ڴ�\n");
	free(buffer);
	buffer_size = 0;
	buffer_count = 0;
	mutex = 1;
	next_pid = 1;
	N = 1; 
	PCB *p;
	printf("������������У�");
	while(ready_queue.front != NULL){
		p = ready_queue.front;
		ready_queue.front = p -> next;
		printf("P%d ",p -> pid);
		free(p);
	}
	printf("���ڴ�\n");
	printf("����������ߵȴ����У�");
	while(producer_queue.front != NULL){
		p = producer_queue.front;
		producer_queue.front = p -> next;
		printf("P%d ",p -> pid);
		free(p);
	}
	printf("���ڴ�\n");
	printf("����������ߵȴ����У�");
	while(consumer_queue.front != NULL){
		p = consumer_queue.front;
		consumer_queue.front = p -> next;
		printf("P%d ",p -> pid);
		free(p);
	}
	printf("���ڴ�\n");
	printf("�����ִ����Ľ�������");
	while(over_list.front != NULL){
		p = over_list.front;
		over_list.front = p -> next;
		printf("P%d ",p -> pid);
		free(p);
	}
	printf("���ڴ�\n\n");
} 

// ��ʼ�����У������е�ͷָ���βָ�붼��ΪNULL
void init_queue(Queue *q) {
    q->front = q->rear = NULL;
}

// ��Ӳ�������һ�����̿��ƿ���뵽���е�β����β�巨 
void enqueue(Queue *q, PCB *proc) {
    proc->next = NULL;  // ���½��̵���һ��ָ����ΪNULL
    if (q->rear == NULL) {  // �������Ϊ��
        q->front = q->rear = proc;  // ���½��̼��Ƕ��е�ͷҲ��β
    } else {
        q->rear->next = proc;  // ���½������ӵ����е�β��
        q->rear = proc;  // ���¶��е�βָ��
    }
}

// ���Ӳ������Ӷ��е�ͷ���Ƴ�һ�����̿��ƿ鲢����
PCB* dequeue(Queue *q) {
    if (q->front == NULL) return NULL;  // �������Ϊ�գ�����NULL
    PCB *proc = q->front;  // ������е�ͷ����
    q->front = q->front->next;  // ���¶��е�ͷָ��
    if (q->front == NULL) q->rear = NULL;  // ������б�գ�����βָ��
    proc->next = NULL;  // �����ӽ��̵���һ��ָ����ΪNULL
    return proc;  // ���س��ӵĽ���
}

// P������������Դ����ģ���ź�����P����
void P(int *semaphore, PCB *p) {
    (*semaphore)--;  // �ź�����1
    if (*semaphore < 0 && p != NULL) {  // ����ź���С��0��˵����Դ����
        if (p->type == PRODUCER) {  // ����������߽���
            enqueue(&producer_queue, p);  // ������������ߵȴ�����
        } 
		else if (p->type == CONSUMER) {
            enqueue(&consumer_queue, p);  // ������������ߵȴ�����
        }
    }
}

// V�������ͷ���Դ����ģ���ź�����V����
void V(int *semaphore, PCB *p) {
    (*semaphore)++;  // �ź�����1
    if (*semaphore <= 0 && p != NULL) {  // ����ź���С�ڵ���0��˵���н����ڵȴ���Դ
        PCB *proc = NULL;
        if (producer_queue.front && p->type == CONSUMER) {  // ��������ߵȴ��������н���
            proc = dequeue(&producer_queue);  // �������ߵȴ�������ȡ��һ������
            printf("     empty <= 0�����������ߵȴ������������� P%d\n", proc->pid);  // ���������Ϣ
        } else if (consumer_queue.front && p->type == PRODUCER) {  // ��������ߵȴ��������н���
            proc = dequeue(&consumer_queue);  // �������ߵȴ�������ȡ��һ������
            printf("     full <= 0�����������ߵȴ������������� P%d\n", proc->pid);  // ���������Ϣ
        }
        if(proc){
        	proc->t = true;
        	enqueue(&ready_queue, proc);
		}
        printf("��ǰ���������б�:");
    	proc = ready_queue.front;
    	if(proc == NULL)printf("��");
   		while (proc != NULL) {
        	printf("P%d ", proc->pid);
        	proc = proc->next;
    	}
    	printf("\n");
        /*if (proc) { // �����ѵĽ��̼���������У�ͷ�巨 
			proc->next = ready_queue.front;
			ready_queue.front = proc;
			if(ready_queue.rear == NULL)ready_queue.rear = proc;
			PCB *current = dequeue(&ready_queue);  // �Ӿ���������ȡ��һ������
			if(proc->type == PRODUCER) {
				if (buffer_count < buffer_size) {  // ������������пռ�
        			buffer[buffer_count++] = proc->product;  // ����Ʒ���뻺�����������ӻ���������
        			printf("P%d�������ߣ������˲�Ʒ %c ��������״̬: [ ", proc->pid, proc->product);
        			for (int i = 0; i < buffer_count; i++) printf("%c ", buffer[i]);  // ���������״̬
        			printf("]\n");
        			enqueue(&over_list, proc);  // ��������ϵĽ��̼���over����
    			} 
			}
			else{
				if (buffer_count > 0) {  // ������������в�Ʒ
        			char product = buffer[--buffer_count];  // �ӻ�����ȡ��һ����Ʒ�������ٻ���������
        			printf("P%d�������ߣ������˲�Ʒ %c ��������״̬: [ ", proc->pid, product);
        			for (int i = 0; i < buffer_count; i++) printf("%c ", buffer[i]);  // ���������״̬
        			printf("]\n");
        			enqueue(&over_list, proc);  // ��������ϵĽ��̼���over����
    			} 
			}
		}*/
    }
}

void show(void); // �������� 

// �������߼���ʵ��������������Ʒ�Ĳ���
void produce(PCB *proc) {
	if(proc->t) goto J;
	P(&empty, proc); // ͬ�������ڻ���ǰ��ֹ��������
	printf("ִ��%d:\tempty = %d",N++,empty);
	if(empty < 0){
		printf("\n    ��P����������empty < 0��˯�ߣ����̽���ȴ����У�\nP%d�������ߣ��򻺳����������������������ߵȴ����У�", proc->pid);  // ���������Ϣ");
		PCB *p = producer_queue.front;
		if(p==NULL)printf("��");
    	while (p != NULL) {
        	printf("P%d ", p->pid);
        	p = p->next;
    	}
    	printf("\n");
	}else{
		J:
		if(proc->t) printf("ִ��%d:\t",N++);
		P(&mutex,NULL);  // ���⣺�����ٽ���������������������Դ
		if(!proc->t) printf("��");
		printf("mutex = %d\n    ��P����������",mutex);
		printf("empty >= 0�������ٽ�����\n");
		if (buffer_count >= buffer_size){
			enqueue(&producer_queue, proc);  // ������������ߵȴ�����
			printf("�����������������������ߵȴ����У�");
			PCB *p = producer_queue.front;
			if(p==NULL)printf("��");
    		while (p != NULL) {
        		printf("P%d ", p->pid);
        		p = p->next;
    		}
    		printf("\n");
		}
		if (buffer_count < buffer_size) {  // ������������пռ�
        buffer[buffer_count++] = proc->product;  // ����Ʒ���뻺�����������ӻ���������
        printf("P%d�������ߣ������˲�Ʒ %c ��������״̬: [ ", proc->pid, proc->product);
        for (int i = 0; i < buffer_count; i++) printf("%c ", buffer[i]);  // ���������״̬
        printf("]\n");
        enqueue(&over_list, proc);  // ��������ϵĽ��̼���over����
    	} 
    	V(&mutex,NULL);  // �˳��ٽ������ͷ���Դ
    	V(&full, proc);  // V����������������������V������˳����Ե��� 
    	printf("\tmutex = %d��full = %d\n    ��V����������",mutex,full);
    	printf("�˳��ٽ������ͷ���Դ��\n");
	}
    show();
    printf("\n��ǰ������״̬: [ ");
    for (int i = 0; i < buffer_count; i++) printf("%c ", buffer[i]);  // ���������״̬
    printf("]\n");
    printf("���ź�����ֵ��mutex = %d��empty = %d��full = %d\n\n",mutex,empty,full);
}

// �������߼���ʵ�����������Ѳ�Ʒ�Ĳ���
void consume(PCB *proc) {
	if(proc->t) goto j;
	P(&full, proc);  // ͬ�������ڻ���ǰ��ֹ��������
	printf("ִ��%d:\tfull = %d",N++,full);
	if(full < 0){
		printf("\n    ��P����������full < 0��˯�ߣ����̽���ȴ����У�\nP%d�������ߣ��򻺳����ձ����������������ߵȴ����У�", proc->pid);  // ���������Ϣ");
		PCB *p = consumer_queue.front;
		if(p==NULL)printf("��");
    	while (p != NULL) {
        	printf("P%d ", p->pid);
        	p = p->next;
    	}
    	printf("\n");
	}else{
		j:
		if(proc->t) printf("ִ��%d:\t",N++);
		P(&mutex,NULL);  // ���⣺�����ٽ���������������������Դ
		if(!proc->t) printf("��");
		P(&mutex,NULL);  // ���⣺�����ٽ���������������������Դ
		printf("mutex = %d\n    ��P����������",mutex);
		printf("full >= 0�������ٽ�����\n");
		if (buffer_count == 0){
			enqueue(&consumer_queue, proc);  // ������������ߵȴ�����
			printf("��������Ϊ�գ����������ߵȴ����У�");
			PCB *p = consumer_queue.front;
			if(p==NULL)printf("��");
    		while (p != NULL) {
        		printf("P%d ", p->pid);
        		p = p->next;
    		}
    		printf("\n");
		}
		if (buffer_count > 0) {  // ������������в�Ʒ
        	char product = buffer[--buffer_count];  // �ӻ�����ȡ��һ����Ʒ�������ٻ���������
        	printf("P%d�������ߣ������˲�Ʒ %c ��������״̬: [ ", proc->pid, product);
        	for (int i = 0; i < buffer_count; i++) printf("%c ", buffer[i]);  // ���������״̬
        	printf("]\n");
        	enqueue(&over_list, proc);  // ��������ϵĽ��̼���over����
    	} 
    	V(&mutex,NULL);  // �˳��ٽ������ͷ���Դ
    	V(&empty, proc); // V����������������������V������˳����Ե���
    	printf("\tmutex = %d��empty = %d\n    ��V����������",mutex,empty);
    	printf("�˳��ٽ������ͷ���Դ��\n");
	}
    show();
    printf("\n��ǰ������״̬: [ ");
    for (int i = 0; i < buffer_count; i++) printf("%c ", buffer[i]);  // ���������״̬
    printf("]\n");
	printf("���ź�����ֵ��mutex = %d��empty = %d��full = %d\n\n",mutex,empty,full);
}

// ������ʵ�֣�������Ⱦ��������еĽ���
void schedule() {
    while (ready_queue.front != NULL) {  // ֻҪ�����������н���
        PCB *current = dequeue(&ready_queue);  // �Ӿ���������ȡ��һ������
        if (current->type == PRODUCER) {  // ����������߽���
            produce(current);  // ִ����������
        } else {
            consume(current);  // ִ�����Ѳ���
        }
        printf("\n");
    }
}

// ��ӡover�����Լ��������������ߵȴ��еĽ���
void show(){
	printf("\n���������б�:");
    PCB *p = ready_queue.front;
    if(p==NULL)printf("��");
    while (p != NULL) {
        printf("P%d ", p->pid);
        p = p->next;
    }
	printf("\n�ѽ����Ľ����б�:");
    p = over_list.front;
    if(p==NULL)printf("��"); 
    while (p != NULL) {
        printf("P%d ", p->pid);
        p = p->next;
    }
    printf("\n�����ߵȴ��б�:");
    p = producer_queue.front;
    if(p==NULL)printf("��");
    while (p != NULL) {
        printf("P%d ", p->pid);
        p = p->next;
    }
    printf("\n�����ߵȴ��б�:");
    p = consumer_queue.front;
    if(p==NULL)printf("��");
    while (p != NULL) {
        printf("P%d ", p->pid);
        p = p->next;
    }
}

int main() {
    // �û��Զ��建������С
    Target:
    system("cls");
	printf("\n    -----------�����ߺ�����������-----------\n");
    printf("\n�����뻺������С: ");
    while (scanf("%d", &buffer_size) != 1 || buffer_size <= 0) {  // ��������Ƿ���Ч
        printf("������Ч������������������: ");
        while (getchar() != '\n'); // ������뻺����
    }
    while (getchar() != '\n');// ������뻺����
    buffer = (char*)malloc(buffer_size * sizeof(char)); // ��̬���仺�����ڴ�
    empty = buffer_size - buffer_count;
	full = buffer_count;
    // ��ʼ������
    init_queue(&ready_queue);
    init_queue(&producer_queue);
    init_queue(&consumer_queue);
    init_queue(&over_list);
    char choice[3];
    do {
        // �û�����������ͺͲ�Ʒ
        int type;
        char product = ' ';
        printf("\n����������������ͣ�0�������ߣ�1�������ߣ�: ");
        while (scanf("%d", &type) != 1 || (type != 0 && type != 1)) {  // ��������Ƿ���Ч
            printf("������Ч�����������루0�������ߣ�1�������ߣ�: ");
            while (getchar() != '\n');// ������뻺����
        }
		/*if (type == 0) {
        	while (getchar() != '\n');// ������뻺����
            printf("���������Ĳ�Ʒ�ַ�: ");
            while (scanf(" %c", &product) != 1) { // ע��ո������������з�
                printf("������Ч�������������ַ�: ");
                while (getchar() != '\n');
            }
        }*/
        if (type == 0) product = '*';
        while (getchar() != '\n');// ������뻺����
        // �������̲������������
        PCB *proc = (PCB*)malloc(sizeof(PCB));
        proc->pid = next_pid++;
        proc->type = (type == 0) ? PRODUCER : CONSUMER;
        proc->product = product;
        proc->t = false;
        enqueue(&ready_queue, proc);
        printf("\n    �ɹ����� "); 
        printf("P%d��%s�� ��", proc->pid, proc->type == PRODUCER ? "������" : "������");
        if(proc->type == PRODUCER)printf("������Ʒ %c\n",proc->product);
        else printf("���Ѳ�Ʒ %c\n",proc->product);
        printf("\n\nҪ���������һ���������Ľ�����( y�� ��/�����ַ��� �� ): ");
        cin >> choice;
        while (getchar() != '\n');// ������뻺����
    } while (choice[0] == 'y' || choice[0] == 'Y');
	//��ӡ��ǰ�û�����ľ������� 
	PCB *p = ready_queue.front;
	system("cls");
	printf("\nĿǰ��ʼ�������еĽ��̣�\n");
    while (p != NULL) {
        printf("P%d��%s�� ��", p->pid, p->type == PRODUCER ? "������" : "������");
        if(p->type == PRODUCER)printf("������Ʒ %c\n",p->product);
        else printf("���Ѳ�Ʒ %c\n",p->product);
        p = p->next;
    }
    printf("\n��ʼ״̬����������СΪ %d����mutex = %d��empty = %d��full = %d\n����empty < 0ʱ�������ֵ��ʾ�����ߵȴ�������Ľ���������full < 0ʱ�������ֵ��ʾ�����ߵȴ�������Ľ�������\n���ڿ�ʼ���н��̵���......\n\n",buffer_size,mutex,empty,full);
    schedule();  // ִ�е���
    //show();    // ��ӡover�����Լ��������������ߵȴ��еĽ���
    reset();    // �ͷŶ�̬������ڴ沢���ò���
    system("pause"); 
    printf("\n\nҪ�����������̵�����( y�� ��/�����ַ��� �� ): ");
    cin >> choice;
    while (getchar() != '\n');// ������뻺����
	if(choice[0] == 'y' || choice[0] == 'Y') goto Target;
	printf("\n\n���˳�����");
    return 0;
}
