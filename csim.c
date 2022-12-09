#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <getopt.h>
#include <time.h>

typedef struct cache { //cache 구조체
    long long tag;  //tag비트
    int accessTime; //LRU를 위한 마지막 접근 시간
    bool valid;		//valid값
} cache;

static int l_hits, s_hits , l_miss, s_miss , t_load, t_store, t_cycle; //hits,miss,load,store등 마지막에 출력할 정보들
static int S, E; //세트 수, 라인 수
static int s, b; //s 인덱스 비트 , b 64비트에서 블록의 offset


cache *cacheReset() {		//cache를 모두 공백인 상태로 구성함
    cache *cache1 = (cache *) malloc(S * E * sizeof(cache));
	//E-way를 위한 동적할당
	
    for (int i = 0; i < S; i++) {
        for (int j = 0; j < E; j++) {
            cache line;
            line.valid = 0;				//valid값 = 0
            cache1[i * E + j] = line;	//cache배열에 저장
        }
    }
    return cache1;
}

void accessCache(cache *cache1, long long address, int *time, char identifier) {
    int Idx = (address >> b) & ((1 << s) - 1);
	//인덱스 비트 추출
    int setIdx = Idx * E;

    long long tag = address >> (b + s);
	//태그, 세트, 블록 순서의 형태이기에 block, set만큼 밀기

    int loadTime = *time;
    int lru = loadTime; // LRU를 확인하기 위함

    int emptySlot = -1; //값이 없는 비어있는 공간
    int lruPos = -1;	//LRU의 위치 저장용

    for (int i = 0; i < E; i++) {
        if (cache1[setIdx + i].valid && cache1[setIdx + i].tag == tag) { //valid값이 1이고 tag가 일치하면
            cache1[setIdx + i].accessTime = loadTime; 	//해당 블록 접근 시간 수정
			if (identifier == 'l')
				l_hits++;								//load hits 증가
			else if (identifier == 's')
				s_hits++;								//store hits 증가
            return;
        } else if (!cache1[setIdx + i].valid) {		//valid값이 0이면 비어있는 공간에 추가
            emptySlot = i;
        } else if (cache1[setIdx + i].accessTime < lru) { 	//해당 위치의 access한 시간이 LRU보다 적은 값이면 LRU 교환
            lru = cache1[setIdx + i].accessTime;			//해당 블록을 LRU로 지정
            lruPos = i;										//LRU 위치 저장
        }
    }
    
	
    if (emptySlot != -1) { //Cold miss일 경우
        cache1[setIdx + emptySlot].valid = 1;
        cache1[setIdx + emptySlot].tag = tag;
        cache1[setIdx + emptySlot].accessTime = loadTime;
        if (identifier == 'l')
			l_miss++;								//load miss 증가
		else if (identifier == 's')
			s_miss++;								//store miss 증가
        return;
    } else {
        cache1[setIdx + lruPos].tag = tag;
        cache1[setIdx + lruPos].accessTime = loadTime;
        if (identifier == 'l')
			l_miss++;								//load miss 증가
		else if (identifier == 's')
			s_miss++;								//store miss 증가
        return;
    }
}

int main(int argc, char *argv[]) {

    int opt;
    char *traceFile;
	char identifier;	//load, store 등등 식별자
    long long address;	//주소
    int size, time;
    time = 0;
	
    while ((opt = getopt(argc, argv, "s:E:b:t:")) != -1) {
        switch (opt) {
            case 's':
                s = atoi(optarg);
                S = 1 << s;
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                traceFile = optarg;
                break;
            default:
				return 1;
        }
    }

   	t_load = t_store = l_hits = l_miss = s_hits = s_miss = 0; //load, store, hits, miss 횟수 초기화
    cache *cache1 = cacheReset(); //cache 생성

    FILE *file1;
    file1 = fopen(traceFile, "r"); //trace 파일 열기

    while (fscanf(file1, " %c %llx,%d", &identifier, &address, &size) > 0) { 
		//load, store 와 주소 그리고 크기로 나열되어있는 trace파일의 형태를 읽어옴
		t_cycle++; //전체 사이클 수
		
        if (identifier == 'l') {				//load일 경우
			t_load++;							//전체 로드수 증가
            accessCache(cache1, address, &time, identifier);
        } else if (identifier == 's') {			//store일 경우
			t_store++;							//전체 store수 증가
            accessCache(cache1, address, &time, identifier);
        } 
        time++;
        printf("\n");
    }

    free(cache1);      //cache 메모리 해제
    fclose(file1);  //open한 파일 close

    printf("Total loads:%d\nTotal stores:%d\nLoad hits:%d\nLoad misses:%d\nStore hits:%d\nStore misses:%d\nTotal cycles:%d\n"
		   , t_load, t_store, l_hits, l_miss, s_hits, s_miss, t_cycle);
	
    return 0;
}