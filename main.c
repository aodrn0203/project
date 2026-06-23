/* ============================================================
 *  컴파일: gcc huffman.c -o huffman
 *  실행:   ./huffman     (윈도우는 huffman.exe)
 * ============================================================ */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define MAX_TEXT 10000   /* 입력 텍스트 최대 길이 */
#define MAX_CHARS 256    /* unsigned char 가 가질 수 있는 값의 개수 (0~255)*/ /*한글은 멀티바이트여서 unsigned char을 사용*/

/* ------------------------------------------------------------
 *  1) 허프만 트리의 노드
 *     - 잎(leaf) 노드: 실제 글자(ch)와 그 빈도(freq)를 가진다.
 *     - 내부 노드: 글자는 의미 없고, 두 자식의 빈도 합을 가진다.
 * ------------------------------------------------------------ */
typedef struct Node {
    unsigned char ch;      /* 글자 (잎 노드에서만 의미 있음) */
    int freq;              /* 빈도 (또는 두 자식 빈도의 합) */
    struct Node *left;     /* 왼쪽 자식 (코드 0) */
    struct Node *right;    /* 오른쪽 자식 (코드 1) */
} Node;

/* 노드 하나를 새로 만들어 주는 함수 */
Node* createNode(unsigned char ch, int freq, Node* left, Node* right) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->ch = ch;
    node->freq = freq;
    node->left = left;
    node->right = right;
    return node;
}

/* 잎 노드인지 검사 (자식이 둘 다 없으면 잎) */
int isLeaf(Node* n) {
    return n->left == NULL && n->right == NULL;
}

/* ------------------------------------------------------------
 *  2) 최소 힙 (우선순위 큐)
 *     허프만 트리를 만들 때 "빈도가 가장 작은 노드 둘"을
 *     반복해서 꺼내야 한다. 그걸 빠르게 하려고 최소 힙을 쓴다.
 * ------------------------------------------------------------ */
typedef struct {
    Node* data[MAX_CHARS]; /* 노드 포인터들을 담는 배열 */
    int size;              /* 현재 들어있는 노드 개수 */
} MinHeap;

/* 두 노드 포인터의 위치를 맞바꾼다 */
void swapNode(Node** a, Node** b) {
    Node* t = *a; *a = *b; *b = t;
}

/* 힙에 노드를 넣는다 (위로 올라가며 제자리 찾기) */
void heapPush(MinHeap* h, Node* node) {
    int i = h->size++;
    h->data[i] = node;
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (h->data[parent]->freq <= h->data[i]->freq) break;
        swapNode(&h->data[parent], &h->data[i]);
        i = parent;
    }
}

/* 힙에서 빈도가 가장 작은 노드를 꺼낸다 (아래로 내려가며 제자리 찾기) */
Node* heapPop(MinHeap* h) {
    Node* min = h->data[0];
    h->data[0] = h->data[--h->size];
    int i = 0;
    while (1) {
        int l = 2 * i + 1, r = 2 * i + 2, smallest = i;
        if (l < h->size && h->data[l]->freq < h->data[smallest]->freq) smallest = l;
        if (r < h->size && h->data[r]->freq < h->data[smallest]->freq) smallest = r;
        if (smallest == i) break;
        swapNode(&h->data[i], &h->data[smallest]);
        i = smallest;
    }
    return min;
}

/* ------------------------------------------------------------
 *  3) 허프만 트리 만들기
 *     (1) 빈도가 0보다 큰 글자마다 잎 노드를 만들어 힙에 넣는다.
 *     (2) 힙에서 가장 작은 두 노드를 꺼내 부모로 합친다.
 *     (3) 노드가 하나 남을 때까지 반복 -> 그 노드가 트리의 뿌리.
 * ------------------------------------------------------------ */
Node* buildHuffmanTree(int freq[]) {
    MinHeap heap;
    heap.size = 0;

    for (int i = 0; i < MAX_CHARS; i++) {
        if (freq[i] > 0) {
            heapPush(&heap, createNode((unsigned char)i, freq[i], NULL, NULL));
        }
    }

    if (heap.size == 0) return NULL;   /* 빈 입력이면 트리도 없음 */

    while (heap.size > 1) {
        Node* a = heapPop(&heap);          /* 가장 작은 노드 */
        Node* b = heapPop(&heap);          /* 두 번째로 작은 노드 */
        Node* parent = createNode(0, a->freq + b->freq, a, b);
        heapPush(&heap, parent);
    }

    return heapPop(&heap);  /* 마지막 남은 노드 = 뿌리 */
}

/* ------------------------------------------------------------
 *  4) 코드표 만들기
 *     뿌리에서 잎까지 내려가며 왼쪽=0, 오른쪽=1 을 붙이면
 *     각 글자의 허프만 코드가 된다.
 * ------------------------------------------------------------ */
void generateCodes(Node* root, char* code, int depth, char* codes[]) {
    if (root == NULL) return;

    if (isLeaf(root)) {
        if (depth == 0) {
            /* 글자가 딱 한 종류뿐이면 코드가 비어버린다. "0" 으로 정해 준다. */
            codes[root->ch] = strdup("0");
        } else {
            code[depth] = '\0';
            codes[root->ch] = strdup(code);
        }
        return;
    }

    code[depth] = '0';
    generateCodes(root->left, code, depth + 1, codes);

    code[depth] = '1';
    generateCodes(root->right, code, depth + 1, codes);
}

/* 트리 메모리를 헤제하는 함수 */
void freeTree(Node* root) {
    if (root == NULL) return;
    freeTree(root->left);
    freeTree(root->right);
    free(root);
}

/* 화면 출력 : 안 보이는 글자는 이름으로 보여준다 */
void printChar(unsigned char c) {
    if (c == ' ')       printf("'공백' ");
    else if (c == '\n') printf("'줄바꿈'");
    else if (c == '\t') printf("'탭'  ");
    else if (c < 0x80)  printf("'%c'   ", c);   /* 일반 ASCII 문자 */
    else                printf("[%02X]  ", c);  /* 한글 등 멀티바이트 조각은 16진수로 */
}

/* ------------------------------------------------------------
 *  각 섹션을 따로 출력하는 함수들
 *  (계산은 runHuffman에서 끝내고, 결과만 받아서 출력한다)
 * ------------------------------------------------------------ */

/* [1] 글자별 빈도 & 허프만 코드표 */
void printSectionTable(int freq[], char* codes[]) {
    printf("\n============================================================\n");
    printf("  [1] 글자별 빈도 & 허프만 코드표\n");
    printf("============================================================\n");
    printf("  값(바이트)    빈도   허프만 코드\n");
    printf("  ------------------------------------\n");
    for (int i = 0; i < MAX_CHARS; i++) {
        if (freq[i] > 0) {
            printf("  ");
            printChar((unsigned char)i);
            printf("\t%4d    %s\n", freq[i], codes[i]);
        }
    }
}

/* [2] 실제 압축된 데이터 (16진수 바이트) */
void printSectionPacked(unsigned char* packed, int byteCount) {
    printf("\n============================================================\n");
    printf("  [2] 실제 압축된 데이터 (16진수 바이트)\n");
    printf("============================================================\n  ");
    for (int i = 0; i < byteCount; i++) {
        printf("%02X ", packed[i]);
        if ((i + 1) % 16 == 0) printf("\n  ");
    }
    printf("\n");
}

/* [3] 압축 결과 분석 */
void printSectionAnalysis(int freq[], char* codes[], int len, long totalBits, int byteCount) {
    long originalBits = (long)len * 8;
    double ratio = (1.0 - (double)totalBits / originalBits) * 100.0;

    int distinct = 0;
    long tableBits = 0;
    for (int i = 0; i < MAX_CHARS; i++) {
        if (freq[i] > 0) {
            distinct++;
            tableBits += 8;                       /* 글자 자체 */
            tableBits += 8;                       /* 코드 길이 */
            tableBits += (long)strlen(codes[i]);  /* 코드 비트 */
        }
    }
    long totalWithTable = totalBits + tableBits;
    double ratioWithTable = (1.0 - (double)totalWithTable / originalBits) * 100.0;

    printf("\n============================================================\n");
    printf("  [3] 압축 결과 분석\n");
    printf("============================================================\n");
    printf("  전체 바이트 수    : %d 바이트\n", len);
    printf("  서로 다른 바이트  : %d 종류\n", distinct);
    printf("  ------------------------------------------------------------\n");
    printf("  원본 크기 (ASCII) : %ld 비트  (= %ld 바이트)\n", originalBits, originalBits / 8);
    printf("  압축 후 (데이터)  : %ld 비트  (= %d 바이트)\n", totalBits, byteCount);
    printf("  압축률            : %.1f%% 절약", ratio);
}

/* [4] 복원(압축 해제) 결과 + 검증 */
void printSectionDecoded(Node* root, const char* text, unsigned char* packed,
                         long totalBits, int len) {
    char* decoded = (char*)malloc(len + 1);
    int produced = 0;

    if (isLeaf(root)) {
        for (int i = 0; i < len; i++) decoded[i] = (char)root->ch;
        produced = len;
    } else {
        Node* cur = root;
        for (long i = 0; i < totalBits && produced < len; i++) {
            int bit = (packed[i / 8] >> (7 - (i % 8))) & 1;
            cur = bit ? cur->right : cur->left;
            if (isLeaf(cur)) {
                decoded[produced++] = (char)cur->ch;
                cur = root;
            }
        }
    }
    decoded[produced] = '\0';

    printf("\n============================================================\n");
    printf("  [4] 복원(압축 해제) 결과\n");
    printf("============================================================\n");
    printf("  복원된 텍스트: %s\n", decoded);
    if (strcmp(text, decoded) == 0)
        printf("  검증 결과    : 원본과 100%% 일치\n");
    else
        printf("  검증 결과    : 불일치 (오류 발생)\n");
    printf("============================================================\n");

    free(decoded);
}

/* ============================================================
 *  메인 처리: 텍스트 하나를 받아 전체 과정을 수행
 *  계산을 모두 끝낸 뒤, 사용자가 고른 섹션만 반복 출력한다.
 * ============================================================ */
void runHuffman(const char* text) {
    int len = (int)strlen(text);
    if (len == 0) {
        printf("\n[알림] 입력된 텍스트가 없습니다.\n");
        return;
    }

    /* (1) 빈도 분석 */
    int freq[MAX_CHARS] = {0};
    for (int i = 0; i < len; i++) freq[(unsigned char)text[i]]++;

    /* (2) 허프만 트리 */
    Node* root = buildHuffmanTree(freq);

    /* (3) 코드표 */
    char* codes[MAX_CHARS] = {0};
    char buffer[MAX_CHARS];
    generateCodes(root, buffer, 0, codes);

    /* (4) 실제 압축: 코드들을 이어붙여 비트로 만들고 바이트에 채운다 */
    long totalBits = 0;
    for (int i = 0; i < len; i++)
        totalBits += (long)strlen(codes[(unsigned char)text[i]]);

    int byteCount = (int)((totalBits + 7) / 8);
    unsigned char* packed = (unsigned char*)calloc(byteCount, 1);

    long bitPos = 0;
    for (int i = 0; i < len; i++) {
        char* code = codes[(unsigned char)text[i]];
        for (int j = 0; code[j] != '\0'; j++) {
            if (code[j] == '1')
                packed[bitPos / 8] |= (unsigned char)(1 << (7 - (bitPos % 8)));
            bitPos++;
        }
    }

    /* (5) 어떤 결과를 볼지 골라서 출력 (원하는 것만) */
    char sel[64];
    while (1) {
        printf("\n------------------------------------------------------------\n");
        printf("  어떤 결과를 보시겠습니까?\n");
        printf("    1. 글자별 빈도 & 허프만 코드표\n");
        printf("    2. 실제 압축된 데이터 (16진수)\n");
        printf("    3. 압축 결과 분석\n");
        printf("    4. 복원(압축 해제) 결과\n");
        printf("    0. 메인 메뉴로 돌아가기\n");
        printf("  선택 > ");

        if (!fgets(sel, sizeof(sel), stdin)) break;

        if (sel[0] == '0') break;

        switch (sel[0]) {
            case '1': printSectionTable(freq, codes); break;
            case '2': printSectionPacked(packed, byteCount); break;
            case '3': printSectionAnalysis(freq, codes, len, totalBits, byteCount); break;
            case '4': printSectionDecoded(root, text, packed, totalBits, len); break;
            default:  printf("\n[알림] 0~4 중에서 입력하세요.\n"); break;
        }
    }

    /* 메모리 정리 */
    for (int i = 0; i < MAX_CHARS; i++) if (codes[i]) free(codes[i]);
    free(packed);
    freeTree(root);
}

int main(void) {
    char line[1024];

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    while (1) {
        printf("\n############################################################\n");
        printf("#                                                          #\n");
        printf("#          허프만 코딩 텍스트 압축기  (Huffman)                #\n");
        printf("#                                                          #\n");
        printf("############################################################\n");
        printf("  1. 새 텍스트 압축하기\n");
        printf("  2. 종료\n");
        printf("------------------------------------------------------------\n");
        printf("  선택 > ");

        if (!fgets(line, sizeof(line), stdin)) break;

        if (line[0] == '2') {
            printf("\n프로그램을 종료합니다.\n");
            break;
        }
        if (line[0] != '1') {
            printf("\n[알림] 1 또는 2를 입력하세요.\n");
            continue;
        }

        printf("\n압축할 텍스트를 입력하세요. (입력을 끝내려면 마지막 줄에 점(.) 하나만 입력)\n");
        printf("------------------------------------------------------------\n");

        char text[MAX_TEXT];
        text[0] = '\0';

        while (fgets(line, sizeof(line), stdin)) {
            if (strcmp(line, ".\n") == 0 || strcmp(line, ".") == 0) break;
            if (strlen(text) + strlen(line) < MAX_TEXT - 1) {
                strcat(text, line);
            } else {
                printf("[알림] 텍스트가 너무 깁니다. 여기까지만 처리합니다.\n");
                break;
            }
        }

        int tlen = (int)strlen(text);
        if (tlen > 0 && text[tlen - 1] == '\n') text[tlen - 1] = '\0';

        runHuffman(text); //
    }
    return 0;
}
