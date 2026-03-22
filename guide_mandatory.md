# Philosophers Mandatory Part -- 完全解説ガイド

---

## 目次

1. [食事する哲学者の問題とは](#1-食事する哲学者の問題とは)
2. [必要な前提知識](#2-必要な前提知識)
   - 2.1 [スレッド (pthread)](#21-スレッドとは)
   - 2.2 [Mutex (相互排他)](#22-mutex相互排他とは)
   - 2.3 [データレース](#23-データレースとは)
   - 2.4 [デッドロック](#24-デッドロックとは)
   - 2.5 [時間管理](#25-時間管理)
3. [課題の仕様まとめ](#3-課題の仕様まとめ)
4. [使用可能な関数の詳細解説](#4-使用可能な関数の詳細解説)
5. [プログラム設計](#5-プログラム設計)
   - 5.1 [データ構造の設計](#51-データ構造の設計)
   - 5.2 [全体フロー](#52-全体フロー)
   - 5.3 [哲学者スレッドのルーチン](#53-哲学者スレッドのルーチン)
   - 5.4 [死亡監視 (モニタースレッド)](#54-死亡監視モニタースレッド)
   - 5.5 [フォークの取り方とデッドロック回避](#55-フォークの取り方とデッドロック回避)
   - 5.6 [ログ出力の排他制御](#56-ログ出力の排他制御)
   - 5.7 [精密な時間待ち](#57-精密な時間待ち)
6. [実装の詳細コード例](#6-実装の詳細コード例)
7. [哲学者が1人の場合の特殊処理](#7-哲学者が1人の場合の特殊処理)
8. [よくあるバグと対策](#8-よくあるバグと対策)
9. [テストケース](#9-テストケース)
10. [Makefile](#10-makefile)

---

## 1. 食事する哲学者の問題とは

Dijkstra が1965年に提唱した並行プログラミングの古典的問題。

**設定:**
- N人の哲学者が円形テーブルに座っている
- テーブルの中央にスパゲッティがある
- 哲学者と哲学者の間にフォークが1本ずつ置かれている (フォークは合計N本)
- 哲学者は「考える → フォーク2本取る → 食べる → フォークを置く → 寝る」を繰り返す
- 一定時間食べられないと餓死する

**何が難しいのか:**
- 全員が同時に左のフォークを取ると、誰も右のフォークを取れず **デッドロック** が発生する
- フォークという共有資源を複数スレッドが争奪するため **データレース** が起きうる
- 公平にフォークが行き渡らないと特定の哲学者が **餓死** する

---

## 2. 必要な前提知識

### 2.1 スレッドとは

**プロセス** は独立したメモリ空間を持つ実行単位。
**スレッド** は同一プロセス内でメモリ空間を共有する実行単位。

```
プロセス
├── スレッド1 (哲学者1)  ─┐
├── スレッド2 (哲学者2)  ─┤  同じメモリ空間を共有
├── スレッド3 (哲学者3)  ─┤  (グローバル変数、ヒープなど)
└── メインスレッド        ─┘
```

**スレッドの利点:**
- プロセスより生成コストが低い
- メモリを共有するのでデータの受け渡しが容易

**スレッドの危険:**
- メモリを共有するため、同時に同じ変数を読み書きすると **データレース** が発生する

### 2.2 Mutex(相互排他)とは

Mutex = **Mut**ual **Ex**clusion (相互排他)

ある共有資源に対して、同時に1つのスレッドだけがアクセスできるように制御する仕組み。

```
スレッドA                      スレッドB
    |                              |
    lock(mutex)                    |
    |  ← ロック獲得                |
    |  共有変数を読み書き          lock(mutex)
    |                              |  ← ブロック (待ち)
    unlock(mutex)                  |
    |                              |  ← ロック獲得
    |                              |  共有変数を読み書き
    |                              unlock(mutex)
```

**基本ルール:**
- `lock` したスレッドだけが `unlock` できる
- `lock` 済みの mutex を別のスレッドが `lock` しようとするとブロック(待機)する
- 使い終わったら必ず `destroy` する

### 2.3 データレースとは

2つ以上のスレッドが同じメモリに同時にアクセスし、少なくとも1つが書き込みである場合に発生。

```c
// !! データレースの例 !!
// スレッド1                    // スレッド2
last_meal_time = now();        if (now() - last_meal_time > time_to_die)
                               //   → 読み書きが同時に起こりうる
```

**防止方法:** その変数にアクセスする全箇所を同一の mutex で `lock/unlock` する。

**検出方法:** コンパイル時に `-fsanitize=thread` をつけてテストする。
```bash
cc -fsanitize=thread -g -o philo *.c -lpthread
```

### 2.4 デッドロックとは

2つ以上のスレッドが互いに相手がロックを解放するのを待ち続け、永久に進まなくなる状態。

```
哲学者1: 左フォーク(fork[0])をlock → 右フォーク(fork[1])を待つ...
哲学者2: 左フォーク(fork[1])をlock → 右フォーク(fork[2])を待つ...
哲学者3: 左フォーク(fork[2])をlock → 右フォーク(fork[0])を待つ...
→ 全員が永久に待ち続ける = デッドロック
```

**回避方法 (この課題で最もシンプル):**
- 偶数番号の哲学者は「右 → 左」の順にフォークを取る
- 奇数番号の哲学者は「左 → 右」の順にフォークを取る
- これにより循環待ちが壊れ、デッドロックが防がれる

### 2.5 時間管理

この課題ではミリ秒(ms)単位で時間を管理する。

**1秒 = 1,000ミリ秒(ms) = 1,000,000マイクロ秒(us)**

`gettimeofday` は **マイクロ秒** 単位の時刻を返す。プログラム内では全てをミリ秒に変換して扱う。

---

## 3. 課題の仕様まとめ

### プログラム要件

| 項目 | 内容 |
|------|------|
| プログラム名 | `philo` |
| 提出ファイル | `Makefile, *.h, *.c` (ディレクトリ `philo/` 内) |
| Makefile ルール | `NAME, all, clean, fclean, re` |
| libft | **使用禁止** |
| グローバル変数 | **使用禁止** |
| データレース | **禁止** |

### 引数

```
./philo number_of_philosophers time_to_die time_to_eat time_to_sleep [number_of_times_each_philosopher_must_eat]
```

| 引数 | 説明 |
|------|------|
| `number_of_philosophers` | 哲学者の人数 = フォークの本数 |
| `time_to_die` (ms) | 最後の食事開始(またはシミュレーション開始)からこの時間内に食事を開始しないと死亡 |
| `time_to_eat` (ms) | 食事にかかる時間。この間フォーク2本を保持 |
| `time_to_sleep` (ms) | 睡眠にかかる時間 |
| `number_of_times_each_philosopher_must_eat` | (省略可) 全員がこの回数食べたら終了 |

### ログ出力形式

```
timestamp_in_ms X has taken a fork
timestamp_in_ms X is eating
timestamp_in_ms X is sleeping
timestamp_in_ms X is thinking
timestamp_in_ms X died
```

- `timestamp_in_ms` = シミュレーション開始からの経過ミリ秒
- `X` = 哲学者番号 (1から始まる)
- メッセージ同士は **重なってはいけない** (printf用mutexが必要)
- 死亡メッセージは実際の死亡から **10ms以内** に表示

### Mandatoryパート固有ルール

- 各哲学者 = **1スレッド**
- 各フォーク = **1 mutex** で保護
- 哲学者同士は通信しない
- 哲学者は他の哲学者が死にそうかどうか知らない

---

## 4. 使用可能な関数の詳細解説

### メモリ・I/O系

```c
// メモリを0で初期化 (構造体の初期化に便利)
void *memset(void *s, int c, size_t n);

// 標準出力にフォーマット出力
int printf(const char *format, ...);

// ヒープメモリを確保
void *malloc(size_t size);

// mallocで確保したメモリを解放
void free(void *ptr);

// ファイルディスクリプタに書き込み (printfの代わりにも使える)
ssize_t write(int fd, const void *buf, size_t count);
```

### 時間系

```c
#include <sys/time.h>

// 現在時刻を取得 (マイクロ秒精度)
// tv_sec: 秒, tv_usec: マイクロ秒
int gettimeofday(struct timeval *tv, struct timezone *tz);

// マイクロ秒単位でスリープ (1ms = 1000us)
// 注意: 精度が低い。指定より長くスリープすることがある
int usleep(useconds_t usec);
```

**`gettimeofday` をミリ秒に変換するヘルパー:**
```c
#include <sys/time.h>

// 現在時刻をミリ秒で返す
long long	get_time_ms(void)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return ((long long)tv.tv_sec * 1000 + tv.tv_usec / 1000);
}
```

### スレッド系

```c
#include <pthread.h>

// 新しいスレッドを作成して関数を実行する
// thread: 作成されたスレッドID格納先
// attr: 属性 (NULLでOK)
// start_routine: スレッドが実行する関数 (void *を受け取りvoid *を返す)
// arg: start_routineに渡す引数
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);

// スレッドの終了を待つ (joinするまでリソースが解放されない)
// retval: スレッドの戻り値格納先 (NULLでOK)
int pthread_join(pthread_t thread, void **retval);

// スレッドをデタッチする (終了時にリソースが自動解放される)
// joinとdetachはどちらか一方のみ使う
int pthread_detach(pthread_t thread);
```

### Mutex系

```c
#include <pthread.h>

// mutexを初期化する
// attr: 属性 (NULLでOK)
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutex_attr_t *attr);

// mutexを破棄する (使い終わったら必ず呼ぶ)
int pthread_mutex_destroy(pthread_mutex_t *mutex);

// mutexをロックする (既にロック中なら解放されるまでブロック)
int pthread_mutex_lock(pthread_mutex_t *mutex);

// mutexをアンロックする (ロックしたスレッドが呼ぶ)
int pthread_mutex_unlock(pthread_mutex_t *mutex);
```

**Mutexの典型的な使い方:**
```c
pthread_mutex_t	mutex;

pthread_mutex_init(&mutex, NULL);

// クリティカルセクション
pthread_mutex_lock(&mutex);
// ... 共有変数の読み書き ...
pthread_mutex_unlock(&mutex);

pthread_mutex_destroy(&mutex);
```

---

## 5. プログラム設計

### 5.1 データ構造の設計

```c
/* ===== 哲学者1人分の情報 ===== */
typedef struct s_philo
{
	int				id;             // 哲学者番号 (1~N)
	int				eat_count;      // 食事回数
	long long		last_meal_time; // 最後に食事を開始した時刻 (ms)
	pthread_t		thread;         // このスレッドのID
	pthread_mutex_t	*left_fork;     // 左フォークへのポインタ
	pthread_mutex_t	*right_fork;    // 右フォークへのポインタ
	struct s_data	*data;          // 共有データへのポインタ
}	t_philo;

/* ===== シミュレーション全体の共有データ ===== */
typedef struct s_data
{
	int				num_philos;       // 哲学者の人数
	int				time_to_die;      // 死亡判定時間 (ms)
	int				time_to_eat;      // 食事時間 (ms)
	int				time_to_sleep;    // 睡眠時間 (ms)
	int				must_eat_count;   // 必要食事回数 (-1なら無制限)
	int				someone_died;     // 誰かが死んだフラグ
	int				all_ate_enough;   // 全員食べ終わったフラグ
	long long		start_time;       // シミュレーション開始時刻 (ms)
	pthread_mutex_t	*forks;           // フォーク配列 [num_philos]
	pthread_mutex_t	print_mutex;      // ログ出力用mutex
	pthread_mutex_t	meal_mutex;       // last_meal_time / eat_count 保護用
	pthread_mutex_t	death_mutex;      // someone_died フラグ保護用
	t_philo			*philos;          // 哲学者配列 [num_philos]
}	t_data;
```

**ポイント:**
- `someone_died` をチェックする箇所は全て `death_mutex` で保護する
- `last_meal_time` と `eat_count` は哲学者スレッドとモニタースレッドの両方がアクセスするため `meal_mutex` で保護する
- `forks` 配列の各要素が1本のフォーク = 1つのmutexに対応する

### フォークの割り当て

```
哲学者1: 左 = fork[0], 右 = fork[1]
哲学者2: 左 = fork[1], 右 = fork[2]
哲学者3: 左 = fork[2], 右 = fork[3]
...
哲学者N: 左 = fork[N-1], 右 = fork[0]  ← 円形テーブル
```

```c
// インデックスは0始まり (i = 0 ~ num_philos-1)
philos[i].left_fork  = &forks[i];
philos[i].right_fork = &forks[(i + 1) % num_philos];
```

### 5.2 全体フロー

```
main()
  │
  ├── 1. 引数パース・バリデーション
  │
  ├── 2. データ構造初期化
  │     ├── t_data 初期化
  │     ├── mutex全て初期化 (forks[], print_mutex, meal_mutex, death_mutex)
  │     └── t_philo 配列初期化 (フォーク割り当て)
  │
  ├── 3. シミュレーション開始
  │     ├── start_time = get_time_ms()
  │     ├── 全哲学者の last_meal_time = start_time
  │     └── pthread_create × N (各哲学者スレッド生成)
  │
  ├── 4. 死亡監視ループ (メインスレッドまたは別スレッド)
  │     └── 全哲学者を巡回して死亡チェック + 全員食べ終わりチェック
  │
  ├── 5. pthread_join × N (全スレッド終了待ち)
  │
  └── 6. クリーンアップ
        ├── mutex全てdestroy
        └── メモリ解放
```

### 5.3 哲学者スレッドのルーチン

各スレッドが実行するメイン関数:

```c
void	*philosopher_routine(void *arg)
{
	t_philo	*philo;

	philo = (t_philo *)arg;
	// 偶数番号は少し待ってからスタート (一斉にフォーク争奪を防ぐ)
	if (philo->id % 2 == 0)
		precise_usleep(philo->data->time_to_eat / 2);
	while (!simulation_stopped(philo->data))
	{
		eat(philo);
		if (simulation_stopped(philo->data))
			break ;
		sleep_and_think(philo);
	}
	return (NULL);
}
```

**食事の流れ:**
```c
void	eat(t_philo *philo)
{
	// 1. フォーク2本を取る (デッドロック回避の順序で)
	take_forks(philo);
	// 2. 「X is eating」を出力
	print_status(philo, "is eating");
	// 3. last_meal_time を更新 (meal_mutex で保護)
	update_last_meal(philo);
	// 4. time_to_eat だけ待つ
	precise_usleep(philo->data->time_to_eat);
	// 5. eat_count をインクリメント (meal_mutex で保護)
	increment_eat_count(philo);
	// 6. フォーク2本を戻す
	release_forks(philo);
}
```

**睡眠→思考の流れ:**
```c
void	sleep_and_think(t_philo *philo)
{
	print_status(philo, "is sleeping");
	precise_usleep(philo->data->time_to_sleep);
	print_status(philo, "is thinking");
	// 奇数人数の場合、thinkingで少し待つと公平性が上がる
}
```

### 5.4 死亡監視 (モニタースレッド)

メインスレッド(または別途作成したモニタースレッド)が全哲学者を巡回して死亡判定を行う。

```c
void	monitor_philos(t_data *data)
{
	int	i;

	while (1)
	{
		i = 0;
		while (i < data->num_philos)
		{
			pthread_mutex_lock(&data->meal_mutex);
			if (get_time_ms() - data->philos[i].last_meal_time
				> data->time_to_die)
			{
				pthread_mutex_unlock(&data->meal_mutex);
				print_status(&data->philos[i], "died");
				pthread_mutex_lock(&data->death_mutex);
				data->someone_died = 1;
				pthread_mutex_unlock(&data->death_mutex);
				return ;
			}
			pthread_mutex_unlock(&data->meal_mutex);
			i++;
		}
		// 全員が必要回数食べたかチェック
		if (all_ate_enough(data))
		{
			pthread_mutex_lock(&data->death_mutex);
			data->all_ate_enough = 1;
			pthread_mutex_unlock(&data->death_mutex);
			return ;
		}
		usleep(1000); // 1msおきにチェック (CPU負荷軽減)
	}
}
```

**重要:** 死亡判定は `time_to_die` を**厳密に超えた**時点で行う。「食事を開始した時刻」からの経過時間であり、「食事を終了した時刻」からではない。

### 5.5 フォークの取り方とデッドロック回避

**デッドロック回避の基本戦略: 番号の小さいフォークから先に取る**

```c
void	take_forks(t_philo *philo)
{
	// 常に番号の小さいforkから先にlockする → 循環待ちを防止
	if (philo->id % 2 == 0)
	{
		// 偶数: 右(番号が小さい方)→ 左
		pthread_mutex_lock(philo->right_fork);
		print_status(philo, "has taken a fork");
		pthread_mutex_lock(philo->left_fork);
		print_status(philo, "has taken a fork");
	}
	else
	{
		// 奇数: 左 → 右
		pthread_mutex_lock(philo->left_fork);
		print_status(philo, "has taken a fork");
		pthread_mutex_lock(philo->right_fork);
		print_status(philo, "has taken a fork");
	}
}

void	release_forks(t_philo *philo)
{
	pthread_mutex_unlock(philo->left_fork);
	pthread_mutex_unlock(philo->right_fork);
}
```

**なぜこれでデッドロックが防げるか:**

全員が同じ方向(左→右)にフォークを取ると循環する:
```
哲学者1: fork[0] → fork[1] を待つ
哲学者2: fork[1] → fork[2] を待つ
哲学者3: fork[2] → fork[0] を待つ  ← 循環!
```

偶数/奇数で逆にすると:
```
哲学者1(奇数): fork[0] → fork[1]
哲学者2(偶数): fork[1] → fork[2]  ← 2はfork[2]を先に取ろうとする
哲学者3(奇数): fork[2] → fork[0]
```
哲学者2は `fork[2]` を先に取り、`fork[1]` を待つ。哲学者1が `fork[1]` を持っている間、哲学者2はブロック。哲学者1は `fork[0]` を取れるので食事でき、`fork[1]` を解放 → 哲学者2が進む。循環が壊れる。

### 5.6 ログ出力の排他制御

```c
void	print_status(t_philo *philo, char *status)
{
	long long	timestamp;

	pthread_mutex_lock(&philo->data->death_mutex);
	if (philo->data->someone_died && ft_strcmp(status, "died") != 0)
	{
		pthread_mutex_unlock(&philo->data->death_mutex);
		return ;
	}
	pthread_mutex_unlock(&philo->data->death_mutex);
	timestamp = get_time_ms() - philo->data->start_time;
	pthread_mutex_lock(&philo->data->print_mutex);
	// 再度チェック (lockの間に死亡した可能性)
	pthread_mutex_lock(&philo->data->death_mutex);
	if (!philo->data->someone_died || ft_strcmp(status, "died") == 0)
	{
		pthread_mutex_unlock(&philo->data->death_mutex);
		printf("%lld %d %s\n", timestamp, philo->id, status);
	}
	else
		pthread_mutex_unlock(&philo->data->death_mutex);
	pthread_mutex_unlock(&philo->data->print_mutex);
}
```

**ポイント:**
- `print_mutex` で printf 呼び出しを保護 → メッセージが混ざらない
- 死亡後はメッセージを出力しない (死亡メッセージ自体を除く)
- タイムスタンプは `print_mutex` を取る**前に**計算する (lock待ち時間を含めない)

### 5.7 精密な時間待ち

`usleep` は精度が低い。例えば `usleep(200000)` (200ms) が実際には 201〜210ms かかることがある。これが積み重なると哲学者が不必要に死ぬ。

**解決策: ビジーウェイト+スリープのハイブリッド**

```c
void	precise_usleep(long long ms)
{
	long long	start;

	start = get_time_ms();
	while (get_time_ms() - start < ms)
	{
		usleep(500); // 0.5ms刻みでチェック
	}
}
```

**なぜ `usleep(500)` なのか:**
- `usleep(1000)` (1ms) だと精度が足りないケースがある
- `usleep(100)` だとCPU使用率が上がりすぎる
- `500` はバランスの良い値

---

## 6. 実装の詳細コード例

### 引数パース

```c
int	parse_args(int argc, char **argv, t_data *data)
{
	if (argc < 5 || argc > 6)
		return (1);
	data->num_philos = ft_atoi(argv[1]);
	data->time_to_die = ft_atoi(argv[2]);
	data->time_to_eat = ft_atoi(argv[3]);
	data->time_to_sleep = ft_atoi(argv[4]);
	if (argc == 6)
		data->must_eat_count = ft_atoi(argv[5]);
	else
		data->must_eat_count = -1;
	if (data->num_philos <= 0 || data->time_to_die <= 0
		|| data->time_to_eat <= 0 || data->time_to_sleep <= 0
		|| (argc == 6 && data->must_eat_count <= 0))
		return (1);
	return (0);
}
```

**注意:** `ft_atoi` は自作する必要がある (libft使用禁止)。オーバーフローや不正文字のチェックも行うべき。

### シミュレーション停止チェック

```c
int	simulation_stopped(t_data *data)
{
	int	stopped;

	pthread_mutex_lock(&data->death_mutex);
	stopped = data->someone_died || data->all_ate_enough;
	pthread_mutex_unlock(&data->death_mutex);
	return (stopped);
}
```

このチェック関数は**必ず** mutex で保護する。`someone_died` は別スレッドが書き込む可能性があるため、保護なしで読むとデータレースになる。

### 初期化

```c
int	init_data(t_data *data)
{
	int	i;

	data->someone_died = 0;
	data->all_ate_enough = 0;
	data->forks = malloc(sizeof(pthread_mutex_t) * data->num_philos);
	data->philos = malloc(sizeof(t_philo) * data->num_philos);
	if (!data->forks || !data->philos)
		return (1);
	// フォークmutex初期化
	i = -1;
	while (++i < data->num_philos)
		pthread_mutex_init(&data->forks[i], NULL);
	pthread_mutex_init(&data->print_mutex, NULL);
	pthread_mutex_init(&data->meal_mutex, NULL);
	pthread_mutex_init(&data->death_mutex, NULL);
	// 哲学者初期化
	i = -1;
	while (++i < data->num_philos)
	{
		data->philos[i].id = i + 1;
		data->philos[i].eat_count = 0;
		data->philos[i].left_fork = &data->forks[i];
		data->philos[i].right_fork = &data->forks[(i + 1) % data->num_philos];
		data->philos[i].data = data;
	}
	return (0);
}
```

### クリーンアップ

```c
void	cleanup(t_data *data)
{
	int	i;

	i = -1;
	while (++i < data->num_philos)
		pthread_mutex_destroy(&data->forks[i]);
	pthread_mutex_destroy(&data->print_mutex);
	pthread_mutex_destroy(&data->meal_mutex);
	pthread_mutex_destroy(&data->death_mutex);
	free(data->forks);
	free(data->philos);
}
```

### main

```c
int	main(int argc, char **argv)
{
	t_data	data;
	int		i;

	memset(&data, 0, sizeof(t_data));
	if (parse_args(argc, argv, &data))
		return (write(2, "Error: invalid arguments\n", 25), 1);
	if (init_data(&data))
		return (write(2, "Error: initialization failed\n", 29), 1);
	// シミュレーション開始
	data.start_time = get_time_ms();
	i = -1;
	while (++i < data.num_philos)
	{
		data.philos[i].last_meal_time = data.start_time;
		if (pthread_create(&data.philos[i].thread, NULL,
				philosopher_routine, &data.philos[i]))
			return (cleanup(&data), 1);
	}
	// 死亡監視
	monitor_philos(&data);
	// 全スレッド終了待ち
	i = -1;
	while (++i < data.num_philos)
		pthread_join(data.philos[i].thread, NULL);
	cleanup(&data);
	return (0);
}
```

---

## 7. 哲学者が1人の場合の特殊処理

哲学者が1人の場合、フォークは1本しかない。2本必要なので絶対に食事できず、`time_to_die` 後に死亡する。

```c
void	*philosopher_routine(void *arg)
{
	t_philo	*philo;

	philo = (t_philo *)arg;
	if (philo->data->num_philos == 1)
	{
		print_status(philo, "has taken a fork");
		precise_usleep(philo->data->time_to_die);
		print_status(philo, "died");
		return (NULL);
	}
	// ... 通常のルーチン ...
}
```

**注意:** 1人の場合にleft_forkをlockしてright_forkを待つと、left == right (同じfork) なので**自分自身でデッドロック**する。必ず特殊処理する。

---

## 8. よくあるバグと対策

### 1. 死亡判定が遅れる (10ms制約違反)

**原因:** モニタースレッドの巡回間隔が大きすぎる / usleepが長すぎる

**対策:** モニターの `usleep` を `1000` (1ms) 以下にする。

### 2. シミュレーション開始直後に死亡する

**原因:** スレッド生成に時間がかかり、最初のスレッドが生成されてから最後のスレッドが動き始めるまでに差がある。`last_meal_time` がスレッド生成時に設定されていない。

**対策:** `last_meal_time` をスレッド生成**前**に `start_time` で初期化する。

### 3. 200人の哲学者で動かない

**原因:** スレッド生成のオーバーヘッド。偶数番号の哲学者に初期遅延がない。

**対策:** 偶数番号の哲学者は開始時に `time_to_eat / 2` ms待つ。これで全員が同時にフォーク争奪しなくなる。

### 4. データレース (TSANで検出される)

**よくある箇所:**
- `someone_died` を mutex なしで読む
- `last_meal_time` を mutex なしで読み書きする
- `eat_count` を mutex なしで読み書きする

**対策:** 共有変数へのアクセスは**全て** mutex で保護する。読み取りだけでもロックが必要。

### 5. 哲学者が奇数人数の時に餓死する

**原因:** 奇数人数だと1人が常にフォークを取れず、不公平が蓄積する。

**対策:** `thinking` 時に適切な遅延を入れる。
```c
// 奇数人数の場合、thinkingで調整
if (data->num_philos % 2 != 0)
{
	think_time = data->time_to_eat - data->time_to_sleep;
	if (think_time > 0)
		precise_usleep(think_time);
}
```

### 6. メモリリーク

**対策:** 全てのエラーパスで確保したメモリを解放する。`valgrind` で確認:
```bash
valgrind --leak-check=full ./philo 5 800 200 200
```

---

## 9. テストケース

### 基本動作テスト

```bash
# 哲学者1人: time_to_die後に死亡すること
./philo 1 800 200 200
# → 800ms付近で "1 died" が出力される

# 誰も死なないケース
./philo 5 800 200 200
# → 永久に動き続ける (Ctrl+Cで停止)

# 食事回数指定
./philo 5 800 200 200 7
# → 全員7回食べて停止

# ギリギリ死なない
./philo 4 410 200 200
# → 誰も死なない (eat200 + sleep200 = 400 < 410)

# ギリギリ死ぬ
./philo 4 310 200 100
# → 死亡する (eat200 + sleep100 = 300、しかしフォーク待ちで310を超える)
```

### エッジケーステスト

```bash
# 哲学者2人
./philo 2 800 200 200
# → 交互に食事して生存

# 大人数
./philo 200 800 200 200
# → 全員生存すべき

# time_to_die が time_to_eat と同じ
./philo 2 200 200 200
# → 死亡する可能性が高い (フォーク待ちで遅延)

# 不正な引数
./philo 0 800 200 200    # → エラー
./philo -5 800 200 200   # → エラー
./philo 5 800 200        # → エラー (引数不足)
```

### データレースの検出

```bash
cc -fsanitize=thread -g -o philo *.c -lpthread
./philo 4 410 200 200
# 警告が出たらデータレースがある
```

---

## 10. Makefile

```makefile
NAME = philo

CC = cc
CFLAGS = -Wall -Wextra -Werror

SRC = main.c parse.c init.c routine.c monitor.c utils.c print.c
OBJ = $(SRC:.c=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -lpthread -o $(NAME)

%.o: %.c philo.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
```

**注意:** `-lpthread` を忘れないこと。pthread関数を使うためにリンクが必要。

---

## 補足: 推奨ファイル構成

```
philo/
├── Makefile
├── philo.h          # 構造体定義、プロトタイプ
├── main.c           # main, parse_args
├── init.c           # init_data, cleanup
├── routine.c        # philosopher_routine, eat, sleep_and_think, take_forks
├── monitor.c        # monitor_philos, simulation_stopped
├── utils.c          # get_time_ms, precise_usleep, ft_atoi
└── print.c          # print_status
```
