# Philosophers 前提知識 -- 完全解説書

Linux におけるスレッド・Mutex・並行処理の基礎から、食事する哲学者の問題を解くために必要な全知識を体系的に解説する。

---

## 目次

1. [プロセスとスレッド](#1-プロセスとスレッド)
   - 1.1 [プロセスとは](#11-プロセスとは)
   - 1.2 [スレッドとは](#12-スレッドとは)
   - 1.3 [プロセスとスレッドの違い](#13-プロセスとスレッドの違い)
   - 1.4 [なぜスレッドを使うのか](#14-なぜスレッドを使うのか)
   - 1.5 [Linux におけるスレッドの実体](#15-linux-におけるスレッドの実体)
2. [POSIX Threads (pthread) API](#2-posix-threads-pthread-api)
   - 2.1 [pthread_create -- スレッド生成](#21-pthread_create----スレッド生成)
   - 2.2 [pthread_join -- スレッド合流](#22-pthread_join----スレッド合流)
   - 2.3 [pthread_detach -- スレッド切り離し](#23-pthread_detach----スレッド切り離し)
   - 2.4 [スレッドの終了方法](#24-スレッドの終了方法)
   - 2.5 [スレッドへの引数渡し](#25-スレッドへの引数渡し)
3. [共有メモリと競合状態](#3-共有メモリと競合状態)
   - 3.1 [何が共有され何が共有されないか](#31-何が共有され何が共有されないか)
   - 3.2 [競合状態 (Race Condition)](#32-競合状態-race-condition)
   - 3.3 [データレース (Data Race)](#33-データレース-data-race)
   - 3.4 [クリティカルセクション](#34-クリティカルセクション)
   - 3.5 [アトミック性の問題](#35-アトミック性の問題)
4. [Mutex (相互排他ロック)](#4-mutex-相互排他ロック)
   - 4.1 [Mutex の概念](#41-mutex-の概念)
   - 4.2 [pthread_mutex_init / destroy](#42-pthread_mutex_init--destroy)
   - 4.3 [pthread_mutex_lock / unlock](#43-pthread_mutex_lock--unlock)
   - 4.4 [Mutex の内部動作](#44-mutex-の内部動作)
   - 4.5 [Mutex 使用時のルール](#45-mutex-使用時のルール)
   - 4.6 [よくある Mutex の誤用](#46-よくある-mutex-の誤用)
   - 4.7 [粒度 -- ロックの範囲設計](#47-粒度----ロックの範囲設計)
5. [デッドロック](#5-デッドロック)
   - 5.1 [デッドロックとは](#51-デッドロックとは)
   - 5.2 [デッドロック発生の4条件](#52-デッドロック発生の4条件)
   - 5.3 [デッドロックの具体例](#53-デッドロックの具体例)
   - 5.4 [デッドロック回避の手法](#54-デッドロック回避の手法)
6. [時間管理](#6-時間管理)
   - 6.1 [gettimeofday の仕組み](#61-gettimeofday-の仕組み)
   - 6.2 [usleep の精度問題](#62-usleep-の精度問題)
   - 6.3 [精密タイマーの実装](#63-精密タイマーの実装)
7. [スレッドスケジューリング](#7-スレッドスケジューリング)
   - 7.1 [OS のスケジューラ](#71-os-のスケジューラ)
   - 7.2 [コンテキストスイッチ](#72-コンテキストスイッチ)
   - 7.3 [スレッド数と CPU コア数の関係](#73-スレッド数と-cpu-コア数の関係)
8. [食事する哲学者の問題 -- 理論](#8-食事する哲学者の問題----理論)
   - 8.1 [問題の歴史と定義](#81-問題の歴史と定義)
   - 8.2 [なぜ難しいか](#82-なぜ難しいか)
   - 8.3 [古典的な解法](#83-古典的な解法)
   - 8.4 [この課題で使うべき解法](#84-この課題で使うべき解法)
   - 8.5 [公平性 (Starvation-Freedom)](#85-公平性-starvation-freedom)
9. [デバッグ手法](#9-デバッグ手法)
   - 9.1 [ThreadSanitizer (TSan)](#91-threadsanitizer-tsan)
   - 9.2 [Valgrind (Helgrind / DRD)](#92-valgrind-helgrind--drd)
   - 9.3 [並行バグのデバッグ心得](#93-並行バグのデバッグ心得)
10. [C 言語の補足知識](#10-c-言語の補足知識)

---

## 1. プロセスとスレッド

### 1.1 プロセスとは

プロセスは OS が管理する**実行中のプログラムのインスタンス**。各プロセスは独立したリソースを持つ。

```
プロセス A                     プロセス B
┌──────────────────┐          ┌──────────────────┐
│ コードセグメント   │          │ コードセグメント   │
│ データセグメント   │          │ データセグメント   │
│ ヒープ            │          │ ヒープ            │
│ スタック          │          │ スタック          │
│ ファイルテーブル   │          │ ファイルテーブル   │
│ PID: 1234        │          │ PID: 5678        │
└──────────────────┘          └──────────────────┘
     完全に独立した仮想アドレス空間
```

**プロセスが持つリソース:**

| リソース | 説明 |
|---------|------|
| 仮想アドレス空間 | コード・データ・ヒープ・スタックを含むメモリ空間 |
| ファイルディスクリプタ | 開いているファイル・パイプ・ソケットの一覧 |
| PID | プロセスID。OS が一意に割り振る |
| 環境変数 | 親プロセスから継承された変数群 |
| シグナルハンドラ | シグナル受信時の処理 |
| ページテーブル | 仮想アドレスを物理アドレスに変換するマッピング |

**`fork()` でプロセスを生成すると:**
- 親のアドレス空間がコピーされる (Copy-on-Write)
- 親子は独立したメモリを持つ → 変数を共有できない
- プロセス間通信 (IPC) にはパイプ・共有メモリ・セマフォなどが必要

### 1.2 スレッドとは

スレッドは**プロセス内の実行の流れ**。同一プロセス内のスレッドはメモリ空間を共有する。

```
プロセス (PID: 1234)
┌────────────────────────────────────────┐
│  コードセグメント (共有)                  │
│  データセグメント (共有)                  │
│  ヒープ          (共有)                  │
│                                        │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐│
│  │スレッド1  │ │スレッド2  │ │スレッド3  ││
│  │スタック   │ │スタック   │ │スタック   ││
│  │レジスタ   │ │レジスタ   │ │レジスタ   ││
│  │TID       │ │TID       │ │TID       ││
│  └──────────┘ └──────────┘ └──────────┘│
└────────────────────────────────────────┘
```

**スレッドが固有に持つもの:**

| リソース | 説明 |
|---------|------|
| スタック | 各スレッド独自のスタック (ローカル変数、関数呼び出し履歴) |
| レジスタ | プログラムカウンタ (PC)、スタックポインタ (SP) など |
| TID | スレッドID |
| errno | エラー番号 (スレッドローカル) |

**スレッドが共有するもの:**

| リソース | 説明 |
|---------|------|
| コード領域 | プログラムの命令列 |
| グローバル変数 | `static` 変数やグローバル変数 |
| ヒープ | `malloc` で確保した領域 |
| ファイルディスクリプタ | 開いているファイル |
| シグナルハンドラ | シグナル処理の設定 |

### 1.3 プロセスとスレッドの違い

```
                    プロセス              スレッド
                    ────────            ────────
メモリ空間          独立                 共有
生成コスト          高い (空間コピー)     低い (スタック確保のみ)
通信手段            IPC が必要           変数を直接共有
安全性              クラッシュが他に波及   1スレッドのクラッシュで
                    しない               プロセス全体が死ぬ
同期の必要性        基本不要              必須 (共有データ保護)
```

### 1.4 なぜスレッドを使うのか

1. **並行実行**: 複数の作業を同時に進められる
2. **低コスト**: プロセスより生成・切替が高速
3. **データ共有が容易**: ポインタを渡すだけでデータ共有できる

Philosophers 課題では「各哲学者が独立に思考・食事・睡眠を行う」ため、各哲学者を1スレッドとして表現するのが自然。

### 1.5 Linux におけるスレッドの実体

Linux カーネルはプロセスとスレッドを区別しない。どちらも `task_struct` で管理される。

- `fork()` → 新しい `task_struct` を作り、アドレス空間を**コピー**
- `clone()` (pthread_create の内部) → 新しい `task_struct` を作り、アドレス空間を**共有**

つまり Linux ではスレッドは「アドレス空間を共有する軽量プロセス (LWP: Light Weight Process)」。

```
カーネルの視点:
task_struct (TID=1001, TGID=1000)  ← メインスレッド
task_struct (TID=1002, TGID=1000)  ← スレッド1
task_struct (TID=1003, TGID=1000)  ← スレッド2
                      ↑
              同じ Thread Group ID = 同じプロセス
```

`TGID` (Thread Group ID) がいわゆる PID に相当し、`getpid()` はこの値を返す。

---

## 2. POSIX Threads (pthread) API

POSIX Threads は Unix 系 OS でスレッドを扱う標準 API (IEEE Std 1003.1c)。
ヘッダ: `#include <pthread.h>`, リンク: `-lpthread`

### 2.1 pthread_create -- スレッド生成

```c
int pthread_create(
    pthread_t *thread,                    // [out] スレッドIDの格納先
    const pthread_attr_t *attr,           // 属性 (NULL = デフォルト)
    void *(*start_routine)(void *),       // スレッドが実行する関数
    void *arg                             // start_routine に渡す引数
);
// 戻り値: 成功=0, 失敗=エラーコード (errno ではない)
```

**動作:**
1. 新しいスレッドのスタック領域を確保 (デフォルト 8MB)
2. カーネルに新しい実行コンテキストを登録 (`clone` システムコール)
3. `start_routine(arg)` の実行を開始
4. 呼び出し元はブロックされず、即座に次の行に進む

**重要な注意:**
```c
// !! 危険なコード !!
for (int i = 0; i < N; i++)
{
    pthread_create(&threads[i], NULL, routine, &i);
    //                                         ^^
    // i のアドレスを渡している。しかし i はループで変化する。
    // スレッドが i を読む頃には既に次の値に変わっている可能性がある。
}
```

### 2.2 pthread_join -- スレッド合流

```c
int pthread_join(
    pthread_t thread,    // 待つ対象のスレッドID
    void **retval        // [out] スレッドの戻り値格納先 (NULL可)
);
// 呼び出したスレッドは、対象スレッドが終了するまでブロックされる
```

**動作:**
1. 対象スレッドが `return` または `pthread_exit` するまでブロック
2. 対象スレッドのリソース (スタック等) を解放
3. 戻り値を `retval` に格納

```
メインスレッド        スレッド1         スレッド2
     |                  |                |
     |  create(1)       |                |
     |─────────────────>|                |
     |  create(2)       |  実行中        |
     |──────────────────|───────────────>|
     |                  |                |  実行中
     |  join(1)         |                |
     |  (ブロック)       |  return        |
     |<─────────────────|                |
     |  join(2)                          |
     |  (ブロック)                        |  return
     |<──────────────────────────────────|
     |
     ▼  全スレッド終了、リソース解放済み
```

**join しないとどうなるか:**
- スレッドの戻り値やスタックが解放されず **メモリリーク** になる
- `pthread_detach` した場合は join 不要 (自動で解放される)

### 2.3 pthread_detach -- スレッド切り離し

```c
int pthread_detach(pthread_t thread);
```

- スレッド終了時にリソースを **自動解放** するよう設定
- detach したスレッドは join できない
- Philosophers 課題では通常 **join を使う** (全スレッド終了を待つため)

### 2.4 スレッドの終了方法

```c
// 方法1: start_routine から return する (推奨)
void *routine(void *arg)
{
    // ... 処理 ...
    return (NULL);  // スレッド終了
}

// 方法2: pthread_exit を呼ぶ
void *routine(void *arg)
{
    // ... 処理 ...
    pthread_exit(NULL);  // ここで終了
    // この行には到達しない
}
```

**注意:** メインスレッドで `exit()` を呼ぶとプロセス全体が終了し、全スレッドが即座に消える。

### 2.5 スレッドへの引数渡し

```c
// 正しい方法: 各スレッド専用のデータを渡す
typedef struct s_philo
{
    int     id;
    // ... 他のフィールド ...
}   t_philo;

t_philo philos[N];

for (int i = 0; i < N; i++)
{
    philos[i].id = i + 1;
    pthread_create(&threads[i], NULL, routine, &philos[i]);
    //                                         ^^^^^^^^^^
    // 各スレッドは自分専用の構造体へのポインタを受け取る
    // ループ変数 i ではなく、永続する構造体を渡している
}

void *routine(void *arg)
{
    t_philo *philo = (t_philo *)arg;
    printf("I am philosopher %d\n", philo->id);
    return (NULL);
}
```

---

## 3. 共有メモリと競合状態

### 3.1 何が共有され何が共有されないか

```c
int global_var = 0;           // 共有される (データセグメント)
static int static_var = 0;    // 共有される (データセグメント)

void *routine(void *arg)
{
    int local_var = 0;         // 共有されない (各スレッドのスタック)
    int *heap = malloc(4);     // heap ポインタ自体はスタック上 (非共有)
                               // しかし malloc で確保した領域はヒープ上 (共有)
    static int s = 0;          // 共有される (関数内staticもデータセグメント)
    return (NULL);
}
```

**まとめ:**

| 領域 | 共有 | 例 |
|------|------|----|
| テキスト (コード) | 共有 | 関数の命令列 |
| データ/BSS | 共有 | グローバル変数, static変数 |
| ヒープ | 共有 | malloc で確保した領域 |
| スタック | 非共有 | ローカル変数, 関数引数 |

Philosophers 課題では構造体を `malloc` でヒープに確保し、ポインタを各スレッドに渡す。つまりスレッド間で構造体のフィールドが共有される。

### 3.2 競合状態 (Race Condition)

複数のスレッドが共有リソースに同時にアクセスし、実行順序によって結果が変わる状態。

```c
// 例: 2つのスレッドが同時に counter をインクリメント
int counter = 0;  // 共有変数

void *increment(void *arg)
{
    for (int i = 0; i < 1000000; i++)
        counter++;  // この1行は実は3つの操作
    return (NULL);
}
```

`counter++` は CPU レベルでは3つの操作:
```
1. LOAD:  レジスタ ← counter (メモリから読む)
2. ADD:   レジスタ ← レジスタ + 1
3. STORE: counter ← レジスタ (メモリに書く)
```

**2スレッドが同時に実行すると:**
```
counter = 0 の状態で...

スレッドA              スレッドB              counter (メモリ)
─────────            ─────────            ────────────────
LOAD (0を読む)                               0
                     LOAD (0を読む)          0
ADD  (0+1=1)                                 0
                     ADD  (0+1=1)            0
STORE (1を書く)                              1
                     STORE (1を書く)         1  ← 本来2のはず!

2回インクリメントしたのに結果は 1。更新が1つ失われた。
```

### 3.3 データレース (Data Race)

データレースは競合状態の中でも特に危険な形態。C言語の仕様上 **未定義動作** となる。

**データレースの定義:**
以下の3条件を全て満たすとき発生する:
1. 2つ以上のスレッドが **同じメモリ位置** にアクセスする
2. 少なくとも1つのアクセスが **書き込み**
3. アクセスが **同期されていない** (mutex等で保護されていない)

```c
// !! データレースの例 !!
// Philosophers 課題で最も起きやすいパターン

// モニタースレッド (読み取り)
if (get_time_ms() - philo->last_meal_time > time_to_die)  // 読む
    // 死亡判定

// 哲学者スレッド (書き込み)
philo->last_meal_time = get_time_ms();  // 書く

// → 同時に発生すると未定義動作
```

**データレースと競合状態の違い:**
- **データレース**: 同期なしの同時アクセス。C の仕様上未定義動作。検出ツールで発見可能
- **競合状態**: プログラムの論理的な誤り。同期していても起きうる。検出が難しい

**データレースがないのに競合状態がある例:**
```c
pthread_mutex_lock(&mutex);
int balance = account->balance;     // 読む
pthread_mutex_unlock(&mutex);

// ← ここで他スレッドが balance を変更する可能性!

pthread_mutex_lock(&mutex);
account->balance = balance - amount; // 書く
pthread_mutex_unlock(&mutex);

// データレースはない (各アクセスはmutexで保護)
// しかし2つのlockの間に他スレッドが介入できるため論理的バグ
```

**正しくは1つのロック内で行う:**
```c
pthread_mutex_lock(&mutex);
account->balance -= amount;  // 読み→計算→書きを1つのロック内で
pthread_mutex_unlock(&mutex);
```

### 3.4 クリティカルセクション

共有資源にアクセスするコード領域のこと。同時に1スレッドだけが実行すべき区間。

```c
// クリティカルセクションの例
pthread_mutex_lock(&mutex);
// ─── ここから ─── クリティカルセクション ───
shared_variable++;
// ─── ここまで ─── クリティカルセクション ───
pthread_mutex_unlock(&mutex);
```

**設計原則:**
- クリティカルセクションは **できるだけ短く** する
- ロック内で時間のかかる処理 (I/O, sleep等) をしない
- ロック内で別のロックを取らない (デッドロックの原因)

### 3.5 アトミック性の問題

「アトミック (不可分)」とは、操作が途中で中断されず、他から途中状態が見えないこと。

```c
// int の代入はアトミックか?
int x = 42;  // 多くのアーキテクチャでアトミック... しかし

// C 言語の仕様ではアトミック性は保証されない!
// コンパイラの最適化やメモリモデルの問題がある
// → 共有変数は必ず mutex で保護するのが正しい
```

Philosophers 課題で `someone_died = 1` と書く場合も、mutex なしで安全とは言えない。コンパイラがこの書き込みを遅延させたり、キャッシュに残して他のスレッドから見えない可能性がある。

---

## 4. Mutex (相互排他ロック)

### 4.1 Mutex の概念

Mutex = **Mut**ual **Ex**clusion (相互排他)

部屋のドアに鍵をかけるようなもの:

```
             鍵のかかる部屋 (Mutex で保護された領域)
            ┌──────────────────────────────┐
            │  共有資源                      │
スレッドA →  │  (変数, データ構造など)         │  ← スレッドB は
 (鍵を持つ)  │                              │     鍵が空くまで
            └──────────────────────────────┘     ドアの前で待つ

・鍵を持てるのは一度に1人だけ
・鍵を持っている人だけが部屋に入れる
・出るときは鍵を戻す
```

### 4.2 pthread_mutex_init / destroy

```c
// Mutex の初期化
pthread_mutex_t mutex;
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
// attr: NULL でデフォルト属性 (ほぼ常に NULL で OK)
// 戻り値: 成功=0

// Mutex の破棄
int pthread_mutex_destroy(pthread_mutex_t *mutex);
// ロック中の mutex を destroy するのは未定義動作
// 必ず全スレッドが unlock した後に呼ぶ
```

**ライフサイクル:**
```
init → (lock/unlock を繰り返す) → destroy
```

**初期化の2つの方法:**
```c
// 方法1: 動的初期化 (ヒープ / 配列に使う)
pthread_mutex_t *mutexes = malloc(sizeof(pthread_mutex_t) * N);
for (int i = 0; i < N; i++)
    pthread_mutex_init(&mutexes[i], NULL);

// 方法2: 静的初期化 (グローバル変数に使う。この課題ではグローバル変数禁止なので使わない)
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
```

### 4.3 pthread_mutex_lock / unlock

```c
int pthread_mutex_lock(pthread_mutex_t *mutex);
// - mutex がアンロック状態 → ロックを取得して即座に返る
// - mutex がロック状態     → ロックが解放されるまでブロック (スレッドは停止)

int pthread_mutex_unlock(pthread_mutex_t *mutex);
// - ロックを解放する
// - 待機中のスレッドがあれば1つを起こす
```

**タイムライン例:**
```
時刻    スレッドA                    スレッドB
────    ────────                    ────────
 0ms    lock(M)   → 即座に取得
 1ms    ... 作業中 ...              lock(M) → ブロック (待機)
 2ms    ... 作業中 ...              | (待機中)
 3ms    unlock(M)                  | → ロック取得! 作業開始
 4ms                               ... 作業中 ...
 5ms                               unlock(M)
```

### 4.4 Mutex の内部動作

Linux の pthread mutex は **Futex** (Fast Userspace muTEX) で実装されている。

```
lock の内部動作:
  1. アトミック命令で mutex の状態を確認
  2. アンロック状態 → アトミックにロック状態に変更 (カーネル呼び出しなし! 高速)
  3. ロック状態   → futex() システムコールでカーネルにスレッドの停止を依頼

unlock の内部動作:
  1. アトミック命令で mutex をアンロック状態に変更
  2. 待機中のスレッドがあれば futex() で1つを起こす
```

**ポイント:** 競合がない場合 (1スレッドだけがアクセス)、lock/unlock はユーザ空間のアトミック命令だけで完結し、システムコールが発生しない。そのためオーバーヘッドは極めて小さい。

### 4.5 Mutex 使用時のルール

1. **ロックしたスレッドがアンロックする** -- 他のスレッドが unlock するのは未定義動作
2. **同じ mutex を2回 lock しない** -- デフォルトの mutex で再帰ロックは未定義動作 (デッドロック)
3. **全てのパスで unlock する** -- エラー時の early return で unlock を忘れやすい
4. **init したら必ず destroy する** -- メモリリーク防止
5. **lock 中の mutex を destroy しない** -- 未定義動作

```c
// !! ルール3の違反例 !!
pthread_mutex_lock(&mutex);
if (error_condition)
    return (-1);  // unlock を忘れている! → mutex が永久にロック状態
pthread_mutex_unlock(&mutex);

// 正しい書き方
pthread_mutex_lock(&mutex);
if (error_condition)
{
    pthread_mutex_unlock(&mutex);  // 必ず unlock してから return
    return (-1);
}
pthread_mutex_unlock(&mutex);
```

### 4.6 よくある Mutex の誤用

**誤用1: 保護が不完全**
```c
// !!間違い!! 書き込みだけ保護して読み取りを保護していない
// 書き込み側
pthread_mutex_lock(&mutex);
shared_var = 42;
pthread_mutex_unlock(&mutex);

// 読み取り側
if (shared_var == 42)  // ← mutex なし! データレース!
    do_something();

// 正しくは読み取りも保護する
pthread_mutex_lock(&mutex);
int local = shared_var;
pthread_mutex_unlock(&mutex);
if (local == 42)
    do_something();
```

**誤用2: 異なる mutex で同じ変数を保護**
```c
// !! 間違い !! 同じ変数を別の mutex で保護しても意味がない
pthread_mutex_lock(&mutex_A);
shared_var++;
pthread_mutex_unlock(&mutex_A);

pthread_mutex_lock(&mutex_B);    // mutex_A ではなく mutex_B を使っている
shared_var++;                    // → 排他になっていない!
pthread_mutex_unlock(&mutex_B);
```

**誤用3: ロックの範囲が広すぎる**
```c
// !! 非効率 !!
pthread_mutex_lock(&mutex);
heavy_computation();         // ← ロック中に重い処理
shared_var = result;
pthread_mutex_unlock(&mutex);

// 改善: ロックは必要最小限に
result = heavy_computation();     // ロック外で計算
pthread_mutex_lock(&mutex);
shared_var = result;              // 共有変数の操作だけロック内
pthread_mutex_unlock(&mutex);
```

### 4.7 粒度 -- ロックの範囲設計

**粗いロック (Coarse-grained):**
```c
// 1つの mutex で全てを保護
pthread_mutex_t big_lock;

// どの共有変数にアクセスする時も同じ big_lock を使う
// 利点: 実装が簡単、デッドロックしにくい
// 欠点: 並行性が下がる (他スレッドが常にブロックされる)
```

**細かいロック (Fine-grained):**
```c
// 変数ごとに mutex を分ける
pthread_mutex_t fork_mutexes[N];   // 各フォークに1つ
pthread_mutex_t print_mutex;       // 出力用
pthread_mutex_t meal_mutex;        // 食事時刻用

// 利点: 並行性が高い (異なるフォークは同時に取れる)
// 欠点: デッドロックのリスク、実装が複雑
```

Philosophers 課題では **フォークごとに1つの mutex** が要求されているため、Fine-grained ロックを使う。

---

## 5. デッドロック

### 5.1 デッドロックとは

2つ以上のスレッド/プロセスが互いにロック解放を待ち合い、永久に進めなくなる状態。

```
   スレッドA              スレッドB
      │                     │
      ▼                     ▼
  lock(M1) 成功          lock(M2) 成功
      │                     │
      ▼                     ▼
  lock(M2) ブロック      lock(M1) ブロック
      │                     │
      ▼                     ▼
  M2 は B が持つ         M1 は A が持つ
  B を待つ...            A を待つ...
      │                     │
      └─── 永久に待ち続ける ──┘
```

### 5.2 デッドロック発生の4条件

デッドロックは以下の4つの条件が **全て同時に** 成立したときに発生する (Coffman条件)。逆に言えば、1つでも崩せばデッドロックは起きない。

| 条件 | 説明 | 哲学者問題での対応 |
|------|------|------------------|
| **相互排他** | リソースは同時に1スレッドしか使えない | フォーク = mutex なので不可避 |
| **保持と待機** | リソースを保持したまま別のリソースを待つ | 左フォークを持ったまま右を待つ |
| **横取り不可** | 他スレッドのリソースを奪えない | 他人のフォークは取れない |
| **循環待ち** | スレッドがリングで待ち合う | 全員が左→右の順で取ると循環する |

**循環待ちを崩す** のが最も実装しやすい。

### 5.3 デッドロックの具体例

**食事する哲学者での典型的デッドロック:**

5人の哲学者が全員同時に左のフォークを取る場合:
```
哲学者1: fork[0] をロック → fork[1] を待つ
哲学者2: fork[1] をロック → fork[2] を待つ
哲学者3: fork[2] をロック → fork[3] を待つ
哲学者4: fork[3] をロック → fork[4] を待つ
哲学者5: fork[4] をロック → fork[0] を待つ ← 循環完成!

5 → 4 → 3 → 2 → 1 → 5 ... (待ちの循環)
```

全員が1本のフォークを握ったまま、もう1本を永久に待ち続ける。

**1人の哲学者での自己デッドロック:**
```c
// 哲学者が1人の場合: left_fork == right_fork (同じ mutex)
philo->left_fork  = &forks[0];
philo->right_fork = &forks[(0 + 1) % 1];  // = &forks[0] ← 同じ!

pthread_mutex_lock(philo->left_fork);      // forks[0] をロック
pthread_mutex_lock(philo->right_fork);     // forks[0] を再度ロック → 自己デッドロック!
```

### 5.4 デッドロック回避の手法

#### 手法1: リソース順序付け (Resource Ordering)

全てのリソースに番号を付け、**常に番号の小さい方から先にロックする** ルールを設ける。

```
ルール: fork[小さい番号] → fork[大きい番号] の順でロック

哲学者1: fork[0] → fork[1]  (0 < 1 なので左→右)
哲学者2: fork[1] → fork[2]  (1 < 2 なので左→右)
哲学者3: fork[2] → fork[3]  (2 < 3 なので左→右)
哲学者4: fork[3] → fork[4]  (3 < 4 なので左→右)
哲学者5: fork[0] → fork[4]  (0 < 4 なので右→左 ← ここだけ逆!)

循環が壊れる:
哲学者5 は fork[0] を先に取る
→ 哲学者1 は fork[0] が空くまで待つ
→ 哲学者5 が食事を終えて fork[0] を解放
→ 哲学者1 が進める → ... → 全員が順番に進める
```

#### 手法2: 偶数/奇数による交互スタート

```
偶数番号: 右フォークから取る (= 少し遅らせて開始)
奇数番号: 左フォークから取る

全員が同時に同じ方向を取ることがなくなる
→ 常に少なくとも1人は2本取れる
→ その1人が食事してフォークを戻す
→ 次の1人が2本取れる → ...
```

これは本質的にはリソース順序付けと同じ効果をもたらす。

#### 手法3: 初期遅延 (Stagger Start)

```c
if (philo->id % 2 == 0)
    usleep(time_to_eat * 500);  // 偶数番号は少し遅れてスタート
```

全員同時にフォーク争奪することを防ぐ。手法2と組み合わせる。

---

## 6. 時間管理

### 6.1 gettimeofday の仕組み

```c
#include <sys/time.h>

struct timeval {
    time_t      tv_sec;   // 秒 (Epoch: 1970-01-01 00:00:00 UTC からの経過秒)
    suseconds_t tv_usec;  // マイクロ秒 (0 ~ 999999)
};

int gettimeofday(struct timeval *tv, struct timezone *tz);
// tz は非推奨。NULL を渡す。
```

**ミリ秒への変換:**
```c
long long get_time_ms(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return ((long long)tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

// 例: tv_sec = 1711000000, tv_usec = 500000
// → 1711000000 * 1000 + 500000 / 1000
// → 1711000000000 + 500
// → 1711000000500 (ms)
```

**経過時間の計算:**
```c
long long start = get_time_ms();  // 例: 1711000000500
// ... 何か処理 ...
long long elapsed = get_time_ms() - start;  // 例: 203 (ms)
```

**タイムスタンプの出力:**
```c
// シミュレーション開始からの経過ミリ秒を表示
long long timestamp = get_time_ms() - data->start_time;
printf("%lld %d %s\n", timestamp, philo->id, status);
// 出力例: "203 2 is eating"
```

**なぜ `long long` を使うか:**
- `int` は最大約 2,147,483,647
- ミリ秒単位の Epoch 時刻は約 1,711,000,000,000 → `int` に収まらない
- `long long` は最低 64bit なので十分

### 6.2 usleep の精度問題

```c
#include <unistd.h>

int usleep(useconds_t usec);  // usec マイクロ秒だけスリープ
```

**問題点:**
1. **最低保証**: usleep は「少なくとも usec マイクロ秒停止する」ことだけを保証。実際にはそれ以上かかることが多い
2. **スケジューラ遅延**: スリープ終了後、OS がそのスレッドに CPU を割り当てるまでに時間がかかる
3. **誤差の蓄積**: `usleep(200000)` (200ms) が実際には 201〜210ms かかると、ループのたびに誤差が蓄積

```
要求: 200ms スリープ
実際: 200ms ───→ OS がスレッドを起こす ──→ CPU 割り当て待ち ──→ 再開
                  ^                         ^
                  スケジューラの判断まで     他スレッドが実行中の場合
                  1~10ms の遅延             さらに遅延
```

**Philosophers での影響:**
```
time_to_die = 410ms
eat = 200ms, sleep = 200ms

理論上: eat(200) + sleep(200) = 400ms < 410ms → 間に合う
現実:   eat(205) + sleep(205) = 410ms = time_to_die → ギリギリ死ぬ場合がある!
```

### 6.3 精密タイマーの実装

**ビジーウェイト方式:**
```c
void precise_usleep(long long wait_ms)
{
    long long start;

    start = get_time_ms();
    while (get_time_ms() - start < wait_ms)
        usleep(500);  // 0.5ms ごとに目覚めて確認
}
```

**動作原理:**
```
要求: 200ms スリープ

0ms:   start = now(), usleep(500)
0.5ms: now() - start = 0.5  < 200 → usleep(500)
1.0ms: now() - start = 1.0  < 200 → usleep(500)
...
199.5ms: now() - start = 199.5 < 200 → usleep(500)
200.0ms: now() - start = 200.0 >= 200 → ループ終了!
```

最大誤差は約 0.5〜1ms に収まる (usleep(500) の実際の遅延分)。

**usleep の値の選び方:**

| 値 | 精度 | CPU 使用率 | 用途 |
|----|------|-----------|------|
| `usleep(100)` | ~0.1ms | 高い | 極めて厳密な時間が必要な場合 |
| `usleep(500)` | ~0.5ms | 中程度 | Philosophers に推奨 |
| `usleep(1000)` | ~1ms | 低い | 大まかでよい場合 |

---

## 7. スレッドスケジューリング

### 7.1 OS のスケジューラ

Linux はプリエンプティブなマルチタスク OS。スケジューラがスレッドの実行順序と実行時間を決定する。

```
CPU (例: 4コア)
┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐
│Core0│ │Core1│ │Core2│ │Core3│
│ T1  │ │ T5  │ │ T3  │ │ T7  │  ← 実行中
└─────┘ └─────┘ └─────┘ └─────┘

実行待ちキュー: T2, T4, T6, T8, T9, ...

スケジューラが定期的に (数ms〜数十msごとに):
1. 現在実行中のスレッドを一時停止 (プリエンプション)
2. 待ちキューから優先度の高いスレッドを選ぶ
3. そのスレッドに CPU を割り当てる
```

**Philosophers への影響:**
- 200人の哲学者 = 200スレッドを作っても、CPU コアが4つなら同時に実行されるのは最大4つ
- 残りはキューで待機しており、順番に CPU 時間をもらう
- スレッド数が多いと切り替えオーバーヘッドが増え、時間精度が低下する

### 7.2 コンテキストスイッチ

OS がスレッドを切り替えるとき、現在のスレッドの状態を保存して次のスレッドの状態を復元する。

```
コンテキストスイッチの流れ:
1. 現スレッドのレジスタをメモリに保存
2. 現スレッドの状態を「ready」に変更
3. 次スレッドの状態を「running」に変更
4. 次スレッドのレジスタをメモリから復元
5. 次スレッドの実行を再開

コスト: 数マイクロ秒 (直接コスト) + キャッシュミス (間接コスト)
```

### 7.3 スレッド数と CPU コア数の関係

```
スレッド数 ≤ コア数:
  全スレッドが真に並列に実行される
  コンテキストスイッチが少ない
  時間精度が高い

スレッド数 > コア数:
  一部のスレッドは待機状態
  コンテキストスイッチが頻発
  時間精度が低下する可能性

例: 200人の哲学者, 4コアCPU
  200スレッド / 4コア = 各コアが50スレッドを切り替えて実行
  スケジューラのタイムスライス ~数ms × 50 = 1周に数百ms
  → 一部の哲学者は長時間 CPU を得られず、time_to_die を超えるリスク
  → 偶数/奇数の初期遅延で競合を減らすことが重要
```

---

## 8. 食事する哲学者の問題 -- 理論

### 8.1 問題の歴史と定義

- **1965年**: Edsger Dijkstra が並行プログラミングの教材として考案
- **原題**: 当初は「食事する5人の哲学者」
- **目的**: デッドロック、競合状態、資源の公平な分配の問題を示す

**問題の正確な定義:**
1. N 人の哲学者が円形テーブルに座る
2. 隣接する哲学者の間にフォークが1本 (計 N 本)
3. 食事にはフォークが2本必要
4. 哲学者は「思考 → フォーク取得 → 食事 → フォーク解放 → 睡眠」を繰り返す
5. 一定時間食事しないと死亡する
6. 哲学者同士は通信しない

### 8.2 なぜ難しいか

**問題1: デッドロック**
全員が同時に左フォークを取ると、誰も右フォークを取れず永久に停止。

**問題2: 餓死 (Starvation)**
デッドロックしなくても、特定の哲学者がいつまでもフォークを取れない可能性。

```
哲学者1と哲学者3が交互に食事 → 哲学者2は永久にフォークを取れない

時刻0: 1が食事(fork[0],fork[1]使用), 3が食事(fork[2],fork[3]使用)
       2は fork[1]を待つ
時刻T: 1が終了, 3が終了
       1がすぐ再び食事開始 → 2はまた fork[1] を取れない
```

**問題3: 公平性の保証**
全哲学者が均等に食事できる仕組みが必要。特に奇数人数の場合は1人が常に不利になりやすい。

### 8.3 古典的な解法

**解法1: リソース順序付け (Dijkstra の解法)**
- フォークに番号を付け、小さい番号から先に取る
- 最後の哲学者だけ取る順序が逆になり、循環待ちが壊れる

**解法2: 仲裁者 (Waiter) 解法**
- 中央の仲裁者がフォークの配布を管理
- 同時に N-1 人までしかフォークを取ろうとできない
- Bonus パートのセマフォ解法に近い

**解法3: Chandy/Misra 解法 (1984)**
- フォークに「汚い/綺麗」の状態を持たせる
- 通信ベース。この課題では「哲学者同士は通信しない」ため使えない

### 8.4 この課題で使うべき解法

**偶数/奇数による交互スタート + フォーク取得順の制御**

```
準備:
  奇数番号 (1, 3, 5...): 左フォーク → 右フォーク の順で取る
  偶数番号 (2, 4, 6...): 右フォーク → 左フォーク の順で取る
                          + 開始時に少し遅延

実行:
  時刻 0ms:   奇数番号が食事開始 (即座にフォーク取得)
  時刻 ~half: 偶数番号が食事開始 (遅延後にフォーク取得)
  時刻 eat:   奇数番号が食事終了 → フォーク解放 → 睡眠
  時刻 eat+h: 偶数番号が食事終了 → フォーク解放 → 睡眠
  → 交互にフォークが使われ、デッドロックも餓死もしない
```

### 8.5 公平性 (Starvation-Freedom)

**偶数人数の場合:**
```
4人の場合:
  グループA: 哲学者1, 3 (奇数)
  グループB: 哲学者2, 4 (偶数)

  A が食事 → B が食事 → A が食事 → ... 完全に交互で公平
```

**奇数人数の場合が問題:**
```
5人の場合:
  同時に食事できるのは最大2人 (5本のフォークで2人×2本 = 4本使用)
  1人は必ず待たされる
  → 3サイクルで全員が2回ずつ食べるように調整が必要

  サイクル1: 1, 3 が食事 (2, 4, 5 は待ち)
  サイクル2: 2, 4 が食事 (1, 3, 5 は待ち)
  サイクル3: 5, 1 が食事 (2, 3, 4 は待ち)  ← 5 がやっと食べられる
```

**奇数人数での thinking 遅延:**
```c
// thinking 時に待ち時間を入れて、フォークを譲る
void think(t_philo *philo)
{
    long long think_time;

    print_status(philo, "is thinking");
    if (philo->data->num_philos % 2 != 0)
    {
        // 奇数人数の場合、食事サイクルを安定させるための遅延
        think_time = philo->data->time_to_eat * 2
                     - philo->data->time_to_sleep;
        if (think_time > 0)
            precise_usleep(think_time);
    }
}
```

**なぜ `time_to_eat * 2 - time_to_sleep` なのか:**

奇数人数では3サイクルで全員が食べる必要がある。1サイクルの時間は:
```
1サイクル = eat + sleep + think

全員が均等に食べるには:
  1サイクルの間に他の2人が食事を完了する必要がある
  → 1サイクル >= eat * 2 (2人分の食事時間)
  → eat + sleep + think >= eat * 2
  → think >= eat * 2 - sleep - eat
  → think >= eat - sleep
```

実際には `eat * 2 - sleep` くらいの余裕を持たせると安定する。

---

## 9. デバッグ手法

### 9.1 ThreadSanitizer (TSan)

GCC/Clang に内蔵されたデータレース検出ツール。

**使い方:**
```bash
cc -fsanitize=thread -g -o philo *.c -lpthread
./philo 4 410 200 200
```

**出力例 (データレースが見つかった場合):**
```
==================
WARNING: ThreadSanitizer: data race (pid=12345)
  Write of size 4 at 0x7f... by thread T3:
    #0 philosopher_routine routine.c:42
    #1 <null> <null>

  Previous read of size 4 at 0x7f... by thread T1:
    #0 monitor_philos monitor.c:15
    #1 <null> <null>

  Location is heap block of size 160 at 0x7f...
==================
```

**読み方:**
- `Write of size 4 ... by thread T3`: スレッドT3が4バイトの書き込みをした場所
- `Previous read of size 4 ... by thread T1`: スレッドT1が同じ場所を読んだ
- `routine.c:42` と `monitor.c:15`: データレースが起きたソースコードの行番号
- → この2箇所のアクセスを同じ mutex で保護する必要がある

**注意:**
- TSan はメモリ使用量が5〜10倍になる
- 実行速度も遅くなる
- TSan が検出しなくても「データレースがない」とは限らない (実行パスに依存)
- TSan を有効にした状態での動作タイミングは通常と異なるため、タイミング依存のテストは通常ビルドでも行う

### 9.2 Valgrind (Helgrind / DRD)

**Helgrind: データレース + ロックエラー検出**
```bash
cc -g -o philo *.c -lpthread  # TSan なしでコンパイル
valgrind --tool=helgrind ./philo 4 410 200 200
```

**DRD: データレース検出 (Helgrind の代替)**
```bash
valgrind --tool=drd ./philo 4 410 200 200
```

**メモリリーク検出:**
```bash
valgrind --leak-check=full ./philo 5 800 200 200 3
```

**Helgrind が検出するもの:**
- データレース
- ロック順序の不整合 (デッドロックの可能性)
- 未初期化の mutex の使用
- ロック中の mutex の destroy

### 9.3 並行バグのデバッグ心得

**1. 並行バグは非決定的**
```
同じプログラムを同じ引数で10回実行して:
- 9回正常に動作
- 1回だけデッドロックまたはデータレース

→ 「動いている」は「正しい」を意味しない
→ ツール (TSan, Helgrind) による検証が必須
```

**2. printf デバッグの罠**
```c
// printf を追加するとタイミングが変わり、バグが消えることがある!
printf("DEBUG: philo %d about to take fork\n", id);  // この出力が遅延を生む
pthread_mutex_lock(fork);                              // → タイミングが変わる

// → printf を入れるとバグが再現しなくなる (ハイゼンバグ)
// → TSan のようなツールを使う方が信頼できる
```

**3. ストレステスト**
```bash
# 同じテストを100回実行
for i in $(seq 1 100); do
    timeout 10 ./philo 5 800 200 200 7
    if [ $? -ne 0 ]; then
        echo "FAIL on iteration $i"
    fi
done
```

**4. エッジケースの優先テスト**
```bash
./philo 1 800 200 200      # 1人 (特殊処理が必要)
./philo 2 400 200 200      # 2人 (最小の非自明ケース)
./philo 200 800 200 200    # 大人数 (スケジューリング圧力)
./philo 4 310 200 100      # ギリギリのタイミング
```

---

## 10. C 言語の補足知識

### void ポインタとキャスト

pthread_create のスレッド関数は `void *` を受け取り `void *` を返す。

```c
void *routine(void *arg)
{
    // void * を具体的な型にキャスト
    t_philo *philo = (t_philo *)arg;

    // philo->id, philo->data などにアクセス可能
    printf("Philosopher %d\n", philo->id);

    return (NULL);  // 戻り値は void * (NULL = 特に返すものなし)
}

// 呼び出し側
t_philo philo;
pthread_create(&thread, NULL, routine, &philo);
//                                      ^^^^^^
// &philo は t_philo * だが、void * に暗黙変換される
```

### 構造体のメモリレイアウト

```c
typedef struct s_philo
{
    int             id;             // 4 bytes
    int             eat_count;      // 4 bytes
    long long       last_meal_time; // 8 bytes
    pthread_t       thread;         // 8 bytes (通常)
    pthread_mutex_t *left_fork;     // 8 bytes (ポインタ)
    pthread_mutex_t *right_fork;    // 8 bytes (ポインタ)
    struct s_data   *data;          // 8 bytes (ポインタ)
}   t_philo;

// 配列として確保:
t_philo *philos = malloc(sizeof(t_philo) * num_philos);

// philos[0], philos[1], ... は連続したメモリに配置される
// 各スレッドに &philos[i] を渡せば、各スレッドは自分の構造体にアクセスできる
```

### typedef と関数ポインタ

```c
// pthread_create の第3引数の型:
void *(*start_routine)(void *)

// これは「void *を引数に取り、void *を返す関数へのポインタ」

// つまり以下の関数がそのまま渡せる:
void *my_routine(void *arg)
{
    // ...
    return (NULL);
}

pthread_create(&thread, NULL, my_routine, arg);
//                             ^^^^^^^^^^
// 関数名は自動的に関数ポインタに変換される
```

### エラーハンドリングのパターン

```c
// pthread 関数は errno を使わず、エラーコードを直接返す
int ret = pthread_create(&thread, NULL, routine, arg);
if (ret != 0)
{
    // ret には EAGAIN, EINVAL, EPERM などが入る
    // perror は使えない (errno ではないため)
    write(2, "Error: thread creation failed\n", 31);
    return (1);
}

// malloc は失敗時に NULL を返す
t_philo *philos = malloc(sizeof(t_philo) * n);
if (!philos)
{
    write(2, "Error: malloc failed\n", 21);
    return (1);
}
```

### volatile キーワード (参考)

```c
// volatile はコンパイラの最適化を抑制する
// しかし volatile はスレッド安全性を保証しない!
volatile int flag = 0;  // コンパイラはこの変数をレジスタにキャッシュしない

// 多くの人が volatile でデータレースを防げると誤解しているが:
// - volatile はメモリバリアを提供しない
// - volatile はアトミック性を保証しない
// → Philosophers 課題では volatile は使わず、mutex で保護するのが正解
```

---

## 付録: 概念の関係図

```
               食事する哲学者の問題
                      │
          ┌───────────┼───────────┐
          │           │           │
     共有資源       並行実行      同期
    (フォーク)    (スレッド)     (Mutex)
          │           │           │
          └─────┬─────┘           │
                │                 │
          ┌─────┴──────┐    ┌─────┴──────┐
          │            │    │            │
      データレース  デッドロック  クリティカル  ロック順序
     (同時アクセス) (循環待ち)  セクション    (回避戦略)
          │            │         │            │
          └─────┬──────┘    ┌────┘            │
                │           │                 │
          ┌─────┴──────┐    │           ┌─────┴──────┐
          │            │    │           │            │
       TSan で検出   偶数/奇数    mutex で     リソース
                    交互開始     保護        順序付け
```
