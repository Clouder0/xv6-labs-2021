#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#define NBUCKET 5
#define NKEYS 100000

struct entry {
  int key;
  int value;
  struct entry *next;
  pthread_mutex_t lock;
};
struct entry *table[NBUCKET];
pthread_mutex_t table_lock[NBUCKET];
int keys[NKEYS];
int nthread = 1;


double
now()
{
 struct timeval tv;
 gettimeofday(&tv, 0);
 return tv.tv_sec + tv.tv_usec / 1000000.0;
}

static void 
insert(int key, int value, int num)
{
  struct entry *e = malloc(sizeof(struct entry));
  e->key = key;
  e->value = value;
  pthread_mutex_init(&e->lock, NULL);
  pthread_mutex_lock(&table_lock[num]);
  struct entry **p = &table[num];
  struct entry *n = table[num];
  e->next = n;
  *p = e;
  pthread_mutex_unlock(&table_lock[num]);
}

static 
void put(int key, int value)
{
  int i = key % NBUCKET;

  // is the key already present?
  struct entry *e = 0;
  pthread_mutex_lock(&table_lock[i]);
  e = table[i];
  pthread_mutex_unlock(&table_lock[i]);
  while(e != 0)
  {
    pthread_mutex_t *l = &e->lock;
    pthread_mutex_lock(l);
    if (e->key == key)
    {
      pthread_mutex_unlock(&e->lock);
      break;
    }
    e = e->next;
    pthread_mutex_unlock(l);
  }
  if (e)
  {
    // update the existing key.
    pthread_mutex_lock(&e->lock);
    e->value = value;
    pthread_mutex_unlock(&e->lock);
  }
  else
  {
    // the new is new.
    insert(key, value, i);
  }
}

static struct entry*
get(int key)
{
  int i = key % NBUCKET;

  struct entry *e = 0;
  pthread_mutex_lock(&table_lock[i]);
  e = table[i];
  pthread_mutex_unlock(&table_lock[i]);
  while(e) {
    if (e->key == key)
      break;
    pthread_mutex_t *l = &e->lock;
    pthread_mutex_lock(l);
    e = e->next;
    pthread_mutex_unlock(l);
  }

  return e;
}

static void *
put_thread(void *xa)
{
  int n = (int) (long) xa; // thread number
  int b = NKEYS/nthread;

  for (int i = 0; i < b; i++) {
    put(keys[b*n + i], n);
  }

  return NULL;
}

static void *
get_thread(void *xa)
{
  int n = (int) (long) xa; // thread number
  int missing = 0;

  for (int i = 0; i < NKEYS; i++) {
    struct entry *e = get(keys[i]);
    if (e == 0) missing++;
  }
  printf("%d: %d keys missing\n", n, missing);
  return NULL;
}

int
main(int argc, char *argv[])
{
  for (int i = 0; i < NBUCKET; ++i)
    pthread_mutex_init(&table_lock[i], NULL);
  pthread_t *tha;
  void *value;
  double t1, t0;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s nthreads\n", argv[0]);
    exit(-1);
  }
  nthread = atoi(argv[1]);
  tha = malloc(sizeof(pthread_t) * nthread);
  srandom(0);
  assert(NKEYS % nthread == 0);
  for (int i = 0; i < NKEYS; i++) {
    keys[i] = random();
  }

  //
  // first the puts
  //
  t0 = now();
  for(int i = 0; i < nthread; i++) {
    assert(pthread_create(&tha[i], NULL, put_thread, (void *) (long) i) == 0);
  }
  for(int i = 0; i < nthread; i++) {
    assert(pthread_join(tha[i], &value) == 0);
  }
  t1 = now();

  printf("%d puts, %.3f seconds, %.0f puts/second\n",
         NKEYS, t1 - t0, NKEYS / (t1 - t0));
  //
  // now the gets
  //
  t0 = now();
  for(int i = 0; i < nthread; i++) {
    assert(pthread_create(&tha[i], NULL, get_thread, (void *) (long) i) == 0);
  }
  for(int i = 0; i < nthread; i++) {
    assert(pthread_join(tha[i], &value) == 0);
  }
  t1 = now();

  printf("%d gets, %.3f seconds, %.0f gets/second\n",
         NKEYS*nthread, t1 - t0, (NKEYS*nthread) / (t1 - t0));
  return 0;
}
