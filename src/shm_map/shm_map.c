/**
 *
 * 基于共享内存实现的map
 *
 * @file shm_map.c
 * @author chosen0ne
 * @date 2012-04-10
 */

#include <pthread.h>

#include "shm_map/shm_map.h"

static int MAX_CAPACITY = 1 << 30;	// 桶的最大个数
static int ENTRY_HEADER_SIZE = sizeof(H_entry);
static int INT_SIZE = sizeof(int);

static H_bulk *map_bulk_list;
static int map_bulk_list_len;
static int *_map_size;
static shmmap_log shm_map_log;

/* 获取hash值 */
static int hash(int h);
/* 根据hash值查找在map_bulk_list中的下标 */
static int index_for(int h);
/* 字符串的hash_code */
static int hash_code(const char *str);
static void* get_shm(const char *file, int size, bool* is_inited);

pthread_mutex_t *g_mptr = NULL; // proc lock
void proc_lock(){
    if(g_mptr != NULL) {
        int iErrno = pthread_mutex_lock(g_mptr);
        if (iErrno != 0) {
            if (iErrno == EOWNERDEAD) {
                pthread_mutex_consistent(g_mptr);
                shm_map_log(SHMMAP_LOG_INFO, "[proc_lock]iErrno=%d", iErrno);
            }else{
                shm_map_log(SHMMAP_LOG_ERROR, "[proc_lock]iErrno=%d", iErrno);
            }
        }
    }
}
void proc_unlock(){
    if (g_mptr != NULL) {
        pthread_mutex_unlock(g_mptr);
    }
}

static int
hash(int h){
	h ^= (h >> 20) ^ (h >> 12);
	return h ^ (h >> 7) ^ (h >> 4);
}

static int
hash_code(const char *str){
	int h = 0;
	const char *p = str;
	while(*p != 0){
		h = 31*h + (int)*p ++;
	}
	return h;
}

static int
index_for(int h){
	return h & (map_bulk_list_len - 1);
}

/*
 * 获取共享内存
 * file: 用于mmap的文件
 * size: 申请共享内存的大小
 */
static void*
get_shm(const char *shm_file_name, int size, bool* is_inited){
	int fd;
	void *idx_ptr;
    bool need_init = false;
	fd = shm_open(shm_file_name, O_RDWR | O_CREAT | O_EXCL, 0777);
    if (fd != -1) {
        /* Set the memory object's size */
        if (ftruncate(fd, size) == -1) {
            shm_map_log(SHMMAP_LOG_ERROR, "[get_shm]ftruncate file error. msg: %s, path: %s",
                        strerror(errno), shm_file_name);
            return NULL;
        }
        need_init = true;
    } else if (errno != EEXIST) {
        shm_map_log(SHMMAP_LOG_ERROR, "[get_shm]shm_open file error. msg: %s, path: %s",
                    strerror(errno), shm_file_name);
        return NULL;
    } else {
        shm_map_log(SHMMAP_LOG_INFO, "[get_shm] shm_open: %s\n", strerror(errno));
        fd = shm_open(shm_file_name, O_RDWR, 0777);
        if (fd == -1) {
            shm_map_log(SHMMAP_LOG_ERROR, "[get_shm]Open data file error. msg: %s, path: %s",
                        strerror(errno), shm_file_name);
            return NULL;
        }
    }

	idx_ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (idx_ptr == MAP_FAILED) {
        shm_map_log(SHMMAP_LOG_ERROR, "[get_shm]mmap failed. msg: %s, path: %s",
                    strerror(errno), shm_file_name);
        return NULL;
    }
    g_mptr = idx_ptr;
    idx_ptr = (char*)idx_ptr + sizeof(pthread_mutex_t);
    if(need_init){
        memset(g_mptr, 0, sizeof(pthread_mutex_t));
        pthread_mutexattr_t mattr;
        pthread_mutexattr_init(&mattr);
        pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
        pthread_mutexattr_setrobust(&mattr, PTHREAD_MUTEX_ROBUST);
        pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ERRORCHECK);
        pthread_mutex_init(g_mptr, &mattr);
        pthread_mutexattr_destroy(&mattr);
    }
    *is_inited = !need_init;
	return idx_ptr;
}

static H_entry*
next_entry(H_entry *entry){
	if(entry != NULL && entry->next_offset != NIL)
		return (H_entry *)get_ptr(entry->next_offset);
	return NULL;
}

bool
map_init(int capacity, int mem_size, const char *dat_file_path, shmmap_log log){
	int 	i;
	void 	*p, *mem;
	bool 	is_inited;
	int		*int_ptr;

	shm_map_log = log;
	if(shm_map_log == NULL){
		shm_map_log = default_shmmap_log;
	}

	if(capacity <= 0){
		shm_map_log(SHMMAP_LOG_ERROR, "[map_init]The capacity of map must be greater than 0");
		return false;
	}
	if(capacity > MAX_CAPACITY)
		capacity = MAX_CAPACITY;
	map_bulk_list_len = 1;
	while(map_bulk_list_len < capacity)
		map_bulk_list_len = map_bulk_list_len << 1;

	if(dat_file_path == NULL){
		dat_file_path = DATA_FILE;
	}
	is_inited = false;

	p = get_shm(dat_file_path, sizeof(pthread_mutex_t) + INT_SIZE * 5 + sizeof(H_bulk) * map_bulk_list_len + mem_size, &is_inited);
    int_ptr = (int *)p;
	/**
	 * 索引文件头部：
	 * Bulk list len 	(4bytes)
	 * padding			(4bytes)
	 * Map size			(4bytes)
	 * padding			(4bytes)
	 * Bulk list		(BULK_SIZE * bulk_list_len bytes)
	 * padding			(4bytes)
	 */
	if(is_inited){
		// 已经有数据文件时，直接load
		map_bulk_list_len = *int_ptr++;
		padding(int_ptr++);
		_map_size = int_ptr++;
		padding(int_ptr++);
		map_bulk_list = (H_bulk *)int_ptr;
	}else{
		*int_ptr++ = map_bulk_list_len;
		padding(int_ptr++);
		_map_size = int_ptr++;
		*_map_size = 0;
		padding(int_ptr++);
		map_bulk_list = (H_bulk *)int_ptr;
		// 初始化所有桶
		for(i=0; i<map_bulk_list_len; i++){
			(map_bulk_list+i)->header_offset = NIL;
			(map_bulk_list+i)->tail_offset = NIL;
			(map_bulk_list+i)->size = 0;
		}
	}
	p = (char *)int_ptr + sizeof(H_bulk) * map_bulk_list_len;
	padding(p);
	mem = (char *)p + INT_SIZE;
	if(!m_init((char*)mem, mem_size, log, is_inited)){
		shm_map_log(SHMMAP_LOG_ERROR, "[map_init]Memory pool init error");
		return false;
	}
	return true;
}

void
map_put(const char *k, const char *v, uint32_t v_len){
    proc_lock();
	H_entry *t, *entry;
	char 	*key_ptr, *val_ptr, *old_val = NULL;
	int 	k_len, entry_offset;
	int 	h = hash(hash_code(k));
	H_bulk 	*hdr = &map_bulk_list[index_for(h)];

	// 先查找是否存在该key对应的entry节点
	if(hdr->size != 0){
		t = (H_entry *)get_ptr(hdr->header_offset);
		while(t != NULL){
			key_ptr = (char *)get_ptr(t->key_offset);
			if(t->hash == h && strcmp(key_ptr, k) == 0)
				break;
			t = next_entry(t);
		}
		// 找到该key对应的节点，直接替换value
		if(t != NULL){
			old_val = (char *)get_ptr(t->value_offset);
			val_ptr = (char*)m_alloc(v_len);
			set_mnode_data_by_data(val_ptr, (void *)v, v_len);
			t->value_offset = ptr_offset(val_ptr);
			m_free(old_val);
            proc_unlock();
            return;
		}
	}
	// 直接在tail处添加节点
	entry = (H_entry *)m_alloc(ENTRY_HEADER_SIZE);
	if(entry == NULL){
		shm_map_log(SHMMAP_LOG_ERROR, "[map_put]Can't allocate memory for entry");
        proc_unlock();
        return;
	}
	entry_offset = ptr_offset(entry);
	// init entry node
	entry->hash = h;
	k_len = strlen(k) + 1;
	key_ptr = (char *)m_alloc(k_len);
	val_ptr = (char *)m_alloc(v_len);
    if(key_ptr == NULL || val_ptr == NULL){
        shm_map_log(SHMMAP_LOG_ERROR, "[map_put]Can't allocate memory for key or val");
        proc_unlock();
        return;
    }
	set_mnode_data_by_data((void *)key_ptr, (void *)k, k_len);
	set_mnode_data_by_data((void *)val_ptr, (void *)v, v_len);
	entry->key_offset = ptr_offset(key_ptr);
	entry->value_offset = ptr_offset(val_ptr);
	if(hdr->size == 0){
		hdr->header_offset = hdr->tail_offset = entry_offset;
		entry->prev_offset = NIL;
	}else{
		t = (H_entry *)get_ptr(hdr->tail_offset);
		entry->prev_offset = hdr->tail_offset;
		t->next_offset = entry_offset;
		hdr->tail_offset = entry_offset;
	}
	entry->next_offset = NIL;
	hdr->size++;
	(*_map_size)++;
    proc_unlock();
}

static
H_entry* map_get_entry(const char *k){
	int 	h = hash(hash_code(k));
	H_bulk 	*hdr = &map_bulk_list[index_for(h)];
	char 	*key_ptr;
	H_entry *t;

	if(hdr->size == 0)
		return NULL;
	t = (H_entry *)get_ptr(hdr->header_offset);

	while(t != NULL){
		key_ptr = (char *)get_ptr(t->key_offset);
		if(h == t->hash && strcmp(k, key_ptr) == 0)
			return t;
		t = next_entry(t);
	}
	return NULL;
}

int
map_size(){
	return *_map_size;
}

char*
map_get(const char *k, uint32_t* v_len){
    proc_lock();
	H_entry *t = map_get_entry(k);
	if(t == NULL){
        proc_unlock();
        *v_len = 0;
        return NULL;
    }
    proc_unlock();
    void* val_ptr = get_ptr(t->value_offset);
    *v_len = get_mnode_data_len_by_data(val_ptr);
	return (char*)val_ptr;
}

bool
map_contains(const char *k){
    proc_lock();
	H_entry *t = map_get_entry(k);
    proc_unlock();
	return t != NULL;
}

void
map_iter(key_iter it){
    proc_lock();
	int i;
	H_bulk *hdr;
	H_entry *t;
	char *k, *v;

	for(i=0; i<map_bulk_list_len; i++){
		hdr = map_bulk_list + i;
		if(hdr->size != 0){
			for(t=(H_entry *)get_ptr(hdr->header_offset); t!=NULL; t=next_entry(t)){
				k = (char *)get_ptr(t->key_offset);
				v = (char *)get_ptr(t->value_offset);
				it(k, v);
			}
		}
	}
    proc_unlock();
}
