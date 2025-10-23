#include "DxLib.h"
#include <stdlib.h>
#include <time.h>

const int WIDTH = 960, HEIGHT = 640; // ウィンドウの幅と高さのピクセル数

// ==== 制限時間を実装するための変数
const int LIMIT_TIME = 120; // 制限時間（秒）
int elapsedFrame = 0;       // 経過フレーム数
int remainingSec;


// ==== プレイヤーに関する情報 ====
int playerHP; // プレイヤーの体力（3回当たるとゲームオーバー）
int startPlayerHP = 5;
int playerLevel = 1;
int playerAttack = 1;
int reflectCount = 0;     // 連続反射回数
int maxReflect = 3;       // 3回でレベルアップ
int moveSpeed; // 通常移動速度
int startSpeed = 10;


// ==== 敵に関する情報 ====
struct Enemy {
    int x;
    int y;
    int w;
    int h;
    int vy; //下に降りてくる速度
    int hp; //敵の体力
    int attackTimer; //敵の攻撃間隔用タイマー
    int moveTimer; //下に降りてくる時間間隔
    int sizeType;   // 敵のサイズを決める　0=小, 1=中, 2=大 
    bool alive;
};

const int ENEMY_MAX = 50; //全体の敵数
const int ENEMY_BATCH = 5; // 一度に出す数

Enemy enemies[ENEMY_MAX]; //エネミー構造体の配列

int enemySpawnTimer = 0;   // 敵生成用タイマー
const int ENEMY_SPAWN_INTERVAL = 600; //生成間隔、60につき１秒

bool playStart; //ゲーム開始時に敵を生成するための変数

// ==== エフェクトの情報 ====
struct HitEffect {
    int x, y;       // 表示位置
    int frame;      // 表示中の経過フレーム数
    bool active;    // 有効かどうか
};

const int EFFECT_MAX = 50; // 最大50個まで
HitEffect effects[EFFECT_MAX];



//敵を重ならないように生成するための関数
void EnemySpawn(int i) {
    //ランダムな割合で生成
    int r = rand() % 100;
    if (r < 50) enemies[i].sizeType = 0;      // 小 50%
    else if (r < 80) enemies[i].sizeType = 1; // 中 30%
    else enemies[i].sizeType = 2;             // 大 20%

    switch (enemies[i].sizeType) {
    case 0: // 小さい敵
        enemies[i].w = 60;
        enemies[i].h = 60;
        enemies[i].hp = 1;
        enemies[i].vy = 12;  // 速い
        break;
    case 1: // 中くらいの敵
        enemies[i].w = 90;
        enemies[i].h = 90;
        enemies[i].hp = 3;
        enemies[i].vy = 8;  // 標準
        break;
    case 2: // 大きい敵
        enemies[i].w = 110;
        enemies[i].h = 110;
        enemies[i].hp = 5;
        enemies[i].vy = 5;  // 遅い
        break;
    }

    enemies[i].attackTimer = rand() % 120; // 攻撃タイミングをずらす

    bool overlap;
    do {
        overlap = false;

        enemies[i].x = rand() % (WIDTH - enemies[i].w);
        enemies[i].y = rand() % 80 + 60;

        for (int j = 0; j < i; j++) {
            int dx = abs(enemies[i].x - enemies[j].x);
            int dy = abs(enemies[i].y - enemies[j].y);
            if (dx < enemies[i].w + 20 && dy < enemies[i].h + 20) {
                overlap = true;
                break;
            }
        }
    } while (overlap);

    enemies[i].moveTimer = 0;
    enemies[i].alive = true;
}

// ==== 敵の弾に関する情報 ====
struct Bullet {
    int x, y;
    int vy;       // 縦方向の速度
    bool isHeal; // 攻撃弾(false) or 回復弾(true)
    bool active;  // この弾が存在するか
};

int attakDuration = 135; // 60につき1秒 
int bulletSpeed = 4;

int healthRatio = 5; //回復弾を出す確率

int ballHitCoolTime = 0; // 衝突後のクールタイム（フレーム単位）

const int BULLET_MAX = 100; // 最大100発まで
Bullet bullets[BULLET_MAX];

// ==== スペースキーのジャスト判定用 ====
static int prevSpace = 0;
int nowSpace = CheckHitKey(KEY_INPUT_SPACE);



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

    SetWindowText("テニスゲーム"); // ウィンドウのタイトル
    SetGraphMode(WIDTH, HEIGHT, 32); // ウィンドウの大きさとカラービット数の指定
    ChangeWindowMode(TRUE); // ウィンドウモードで起動
    srand((unsigned int)time(NULL)); //乱数の初期化
    if (DxLib_Init() == -1) return -1; // ライブラリ初期化 エラーが起きたら終了
    SetBackgroundColor(0, 0, 0); // 背景色の指定
    SetDrawScreen(DX_SCREEN_BACK); // 描画面を裏画面にする

    // ボールを動かすための変数
    int ballX = 40;
    int ballY = 80;
    int ballVx = 5;
    int ballVy = 5;
    int ballR = 10;

    // ラケットを動かすための変数
    int racketX = WIDTH / 2;
    int racketY = HEIGHT - 50;
    int racketW = 100;
    int racketH = 30;



    // ゲーム進行に関する変数、スコアを代入する変数
    //シーンを制御するための列挙
    enum { 
        TITLE_Scene, 
        PLAY_Scene, 
        RESULT_Scene,
        GAMEOVER_Scene 
    };
    int scene = TITLE_Scene;
    int timer = 0;
    int score = 0; // スコアを代入
    int highScore = 1000; // ハイスコアを代入
    int dx, dy; // ヒットチェック用の変数宣言

    int imgBg = LoadGraph("image/bg3_2.png"); // 背景画像の読み込み プレイ画面
    int imgTl = LoadGraph("image/tl_2.png"); // タイトル
    int imgRs = LoadGraph("image/rs_1.png"); // リザルト
    int imgGo = LoadGraph("image/go_1.png"); // ゲームオーバー
    int imgBall = LoadGraph("image/bullet_2.png"); // 大砲の弾画像の読み込み
    int imgEnemySlime = LoadGraph("image/slime.png"); // スライム
    int imgEnemyBird = LoadGraph("image/monster2.png"); //謎のモンスターA
    int imgEnemyGolem = LoadGraph("image/golem.png"); //謎のモンスターB
    int imgPlayer = LoadGraph("image/player.png"); //プレイヤー

    // サウンドの読み込みと音量設定
    int titleBgm = LoadSoundMem("sound/maou_bgm_fantasy06.mp3");//タイトル
    int bgm = LoadSoundMem("sound/maou_bgm_fantasy15.mp3");//プレイ画面
    int jin = LoadSoundMem("sound/maou_bgm_piano07.mp3");//ゲームオーバー
    int result = LoadSoundMem("sound/result.mp3");//リザルト
    int atack_Se = LoadSoundMem("sound/atack.mp3");//敵への攻撃
    int damage_Se = LoadSoundMem("sound/damage.mp3");//ダメージ
    int recovery_Se = LoadSoundMem("sound/kaihuku.mp3");//回復
    int powerup_Se = LoadSoundMem("sound/powerUp.mp3");//レベルアップ
    ChangeVolumeSoundMem(128, titleBgm);
    ChangeVolumeSoundMem(128, bgm);
    ChangeVolumeSoundMem(128, result);
    ChangeVolumeSoundMem(128, jin);

    while (1) // メインループ
    {
        ClearDrawScreen(); // 画面をクリアする
        DrawGraph(0, 0, imgBg, FALSE); // 背景の描画
        timer++;
        switch (scene) // タイトル、ゲームをプレイ、ゲームオーバーの分岐
        {
        case TITLE_Scene: // タイトル画面の処理
            DrawGraph(0, 0, imgTl, FALSE); // 背景の描画
            SetFontSize(50);
            DrawString(WIDTH / 2 - 50 / 2 * 12 / 2, HEIGHT / 3, "Enemy Breaker", 0x00ff00);

            // === タイトルBGMの再生 ===
            if (CheckSoundMem(titleBgm) == 0) { // まだ再生されていないときだけ
                PlaySoundMem(titleBgm, DX_PLAYTYPE_LOOP);
            }
            // 文字を点滅表示の処理
            if (timer % 60 < 30) { 
                SetFontSize(30);
                DrawString(WIDTH / 2 - 30 / 2 * 21 / 2, HEIGHT * 2 / 3, "Press SPACE to start.", 0x00ffff);
            }
            if (CheckHitKey(KEY_INPUT_SPACE) == 1) // スペースキー押し下し
            {
                ballX = WIDTH / 2 - 100;
                ballY = HEIGHT / 2 - 120;
                ballVx = 5;
                ballVy = 5;
                racketX = WIDTH / 2;
                racketY = HEIGHT - 50;
                score = 0; //スコアのリセット
                playStart = true;

                //プレイヤーの情報をリセット
                playerHP = startPlayerHP; 
                playerLevel = 1;
                playerAttack = 1;
                reflectCount = 0;

                elapsedFrame = 0; // 制限時間カウンタをリセット
                remainingSec = 0; //残り時間をリセット

                //敵の情報をリセット
                enemySpawnTimer = 0; // 敵の生成タイマーリセット

                for (int i = 0; i < ENEMY_MAX; i++) {
                    enemies[i].alive = false;
                    enemies[i].hp = 0;
                    enemies[i].x = 0;
                    enemies[i].y = 0;
                    enemies[i].moveTimer = 0;
                    enemies[i].attackTimer = 0;
                    enemies[i].sizeType = 0;
                }

                //弾の情報をリセット
                for (int i = 0; i < BULLET_MAX; i++) {
                    bullets[i].active = false;
                }
                
                // エフェクトの情報をリセット
                for (int i = 0; i < EFFECT_MAX; i++) {
                    effects[i].active = false;
                }

                scene = PLAY_Scene; //プレイ画面へ移動
                StopSoundMem(titleBgm);
                PlaySoundMem(bgm, DX_PLAYTYPE_LOOP); // BGMをループ再生
            }
            break;

        // ==== プレイ中の処理 ====
        case PLAY_Scene: 
            DrawGraph(0, 0, imgBg, FALSE); // 背景の描画

            // ==== プレイ中のUI ====
            SetFontSize(20); // 文字の大きさ
            DrawFormatString(10, 10, 0xffffff, "SCORE %d", score); //スコアを表示
            DrawFormatString(170, 10, GetColor(255, 0, 0), "PLAYER HP: %d", playerHP); // プレイヤーのHPを表示
            DrawFormatString(330, 10, GetColor(0, 255, 0), "LEVEL: %d", playerLevel); //プレイヤーのレベルを表示
            DrawFormatString(440, 10, GetColor(0, 200, 255), "ATK: %d", playerAttack); //プレイヤーの攻撃力を表示
            DrawFormatString(530, 10, GetColor(255, 255, 0), "REFLECT: %d / %d", reflectCount, maxReflect); //プレイヤーの反射回数を表示
            DrawFormatString(WIDTH - 250, 10, 0xffff00, "HI-SC %d", highScore); //ハイスコアを表示
            DrawFormatString(WIDTH - 120, 10, GetColor(255, 255, 255), "TIME: %3d", remainingSec); // 制限時間を表示

            // ==== 敵と弾を初期化する処理 ====
            if (playStart) {

                for (int i = 0; i < ENEMY_BATCH; i++) {
                    EnemySpawn(i);
                }
                playStart = false;
            }

            // ==== ボールの処理 ====
            //ボールの判定のクールタイムの処理
            if (ballHitCoolTime > 0) {
                ballHitCoolTime--; // 毎フレーム減らす
            }

            // ボールの移動処理
            ballX = ballX + ballVx;
            if (ballX < ballR && ballVx < 0) ballVx = -ballVx;
            if (ballX > WIDTH - ballR && ballVx > 0) ballVx = -ballVx;
            ballY = ballY + ballVy;
            if (ballY < ballR && ballVy < 0) ballVy = -ballVy;
            //            if (ballY > HEIGHT && ballVy > 0) ballVy = -ballVy;
            
            // ボールが下端に達した時
            if (ballY > HEIGHT) 
            {
                WaitTimer(500); // 0.5秒待機して再開

                //  再生成処理 
                ballX = WIDTH / 2 + (rand() % 200 - 100); // 中央付近ランダム
                ballY = HEIGHT / 2;
                ballVx = (rand() % 2 == 0) ? 5 : -5; // 左右ランダム方向
                ballVy = -5;                         // 上方向へ発射

                reflectCount = 0; // コンボ解除
                playerLevel = 1; // レベルリセット
                playerAttack = 1;
                PlaySoundMem(damage_Se, DX_PLAYTYPE_BACK); // ジングルを出力
                playerHP--;
                break;
            }
            // === 砲弾の画像を描画 ===
            if (imgBall != -1) {

                int ballImgW, ballImgH;
                GetGraphSize(imgBall, &ballImgW, &ballImgH);

                // ボールの中心が (ballX, ballY) になるように描画
                DrawGraph(ballX - ballImgW / 2, ballY - ballImgH / 2, imgBall, TRUE);
            }
            else {
                // 読み込み失敗時の代替（赤い円を描く）
                DrawCircle(ballX, ballY, ballR, GetColor(255, 0, 0), TRUE);
            }


            // ==== 敵生成処理 ====
            enemySpawnTimer++;

            if (enemySpawnTimer >= ENEMY_SPAWN_INTERVAL) {
                enemySpawnTimer = 0; // タイマーリセット

                int spawned = 0; // この周期で何体出したか

                // 空いてるスロットを探して最大5体まで生成
                for (int i = 0; i < ENEMY_MAX && spawned < ENEMY_BATCH; i++) {
                    if (!enemies[i].alive) {

                        EnemySpawn(i);
                        spawned++;
                    }
                }
            }


            // === 敵の移動処理 ===
            for (int i = 0; i < ENEMY_MAX; i++) {
                if (enemies[i].alive) {
                    enemies[i].moveTimer++;
                    enemies[i].attackTimer++;

                    //敵が弾を打つ処理
                    // 一定間隔ごとに1発弾を撃つ
                    if (enemies[i].attackTimer >= attakDuration) { // 約1.5秒ごと
                        enemies[i].attackTimer = 0;

                        // 空きスロットの弾を探す
                        for (int j = 0; j < BULLET_MAX; j++) {
                            if (!bullets[j].active) {
                                bullets[j].x = enemies[i].x + enemies[i].w / 2;
                                bullets[j].y = enemies[i].y + enemies[i].h;
                                bullets[j].vy = bulletSpeed; // 弾の速さ
                                bullets[j].active = true;

                                //  低確率で回復弾を出す
                                int r = rand() % 100;
                                bullets[j].isHeal = (r < healthRatio); // 確率で回復弾

                                break;
                            }
                        }
                    }

                    // 敵が一定時間ごとにゆっくり下降
                    if (enemies[i].moveTimer >= 60) { // 30フレーム＝約0.5秒
                        enemies[i].y += enemies[i].vy;
                        enemies[i].moveTimer = 0;
                    }

                    // 敵を描画する処理
                    for (int i = 0; i < ENEMY_MAX; i++) {
                        if (enemies[i].alive) {
                            int drawX = enemies[i].x;
                            int drawY = enemies[i].y;

                            // 敵の種類ごとに画像を描画
                            switch (enemies[i].sizeType) {
                            case 0: // 小＝スライム
                                DrawExtendGraph(
                                    drawX, drawY,
                                    drawX + enemies[i].w, drawY + enemies[i].h,
                                    imgEnemySlime, TRUE
                                );
                                break;
                            case 1: // 中＝鳥
                                DrawExtendGraph(
                                    drawX, drawY,
                                    drawX + enemies[i].w, drawY + enemies[i].h,
                                    imgEnemyBird, TRUE
                                );
                                break;
                            case 2: // 大＝ゴーレム
                                DrawExtendGraph(
                                    drawX, drawY,
                                    drawX + enemies[i].w, drawY + enemies[i].h,
                                    imgEnemyGolem, TRUE
                                );
                                break;
                            }

                            // HPバー描画
                            int barWidth = (int)(enemies[i].w * ((float)enemies[i].hp / 5)); // HP5を最大幅
                            DrawBox(
                                enemies[i].x, enemies[i].y - 6,
                                enemies[i].x + barWidth, enemies[i].y - 3,
                                GetColor(0, 255, 0), TRUE
                            );
                        }
                    }

                    // 画面下に出たら消す
                    if (enemies[i].y > HEIGHT) {
                        enemies[i].alive = false;
                        scene = GAMEOVER_Scene;
                    }
                }
            }
            
            // ==== 弾を描画する処理 ====
            for (int i = 0; i < BULLET_MAX; i++) {
                if (bullets[i].active) {
                    bullets[i].y += bullets[i].vy; // 下に移動

                    // 弾の描画
                    if (bullets[i].isHeal) {
                        //  回復弾（緑）
                        if (timer % 10 < 5) {
                            DrawCircle(bullets[i].x, bullets[i].y, 6, GetColor(0, 255, 0), TRUE);
                        }
                    }
                    else {
                        //  通常攻撃弾（黄色）
                        DrawCircle(bullets[i].x, bullets[i].y, 5, GetColor(255, 255, 0), TRUE);
                    }

                    // 画面下に出たら消す
                    if (bullets[i].y > HEIGHT) {
                        bullets[i].active = false;
                    }

                    // === プレイヤーと弾の当たり判定 ===
                    int px1 = racketX - racketW / 2;
                    int px2 = racketX + racketW / 2;
                    int py1 = racketY - racketH / 2;
                    int py2 = racketY + racketH / 2;

                    int bx = bullets[i].x;
                    int by = bullets[i].y;
                    int br = 5; // 弾の半径

                    // 矩形と円の重なり判定（簡易）
                    if (bx + br > px1 && bx - br < px2 &&
                        by + br > py1 && by - br < py2)
                    {
                        if (bullets[i].isHeal) {
                            //  回復弾：HP回復
                            if (playerHP < startPlayerHP) {
                                playerHP += 2;
                                //回復エフェクト
                                for (int k = 0; k < 3; k++) {
                                    DrawCircle(racketX, racketY, 40 + k * 10, GetColor(0, 255, 0), FALSE);
                                }
                                PlaySoundMem(recovery_Se, DX_PLAYTYPE_BACK); // 回復音
                            }
                        }
                        else {
                            //  攻撃弾：ダメージ
                            playerHP--;
                            if (score - 500 <= 0) {
                                score = 0;
                            }
                            else {
                                score -= 500;
                            }
                           
                            PlaySoundMem(damage_Se, DX_PLAYTYPE_BACK);
                            WaitTimer(500); // 0.5秒待機して再開

                            if (playerHP <= 0) {
                                scene = GAMEOVER_Scene;
                                timer = 0;
                                StopSoundMem(bgm);
                                PlaySoundMem(jin, DX_PLAYTYPE_BACK);
                            }
                        }
                        bullets[i].active = false;  // 弾を消す
                    }
                }
            }



            // ==== プレイヤーの処理 ====
            // スペースキーを押している間は加速
            if (CheckHitKey(KEY_INPUT_SPACE) == 1) {
                moveSpeed = 18; // 加速時の速度

                // 加算合成でプレイヤーの後ろを光らせる
                SetDrawBlendMode(DX_BLENDMODE_ADD, 128);
                DrawCircle(racketX, racketY, 40, GetColor(100, 200, 255), FALSE);
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
            }
            else {
                moveSpeed = startSpeed;
            }

            if (CheckHitKey(KEY_INPUT_LEFT) == 1) {
                racketX -= moveSpeed;
                if (racketX < racketW / 2) racketX = racketW / 2;
            }
            if (CheckHitKey(KEY_INPUT_RIGHT) == 1) {
                racketX += moveSpeed;
                if (racketX > WIDTH - racketW / 2) racketX = WIDTH - racketW / 2;
            }

            int playerW, playerH;
            GetGraphSize(imgPlayer, &playerW, &playerH);

            // プレイヤーの中心が (racketX, racketY) に来るように描画
            DrawGraph(racketX - playerW / 2, racketY - playerH / 2, imgPlayer, TRUE);

            // プレイヤーの中心が (racketX, racketY) に来るように描画
            DrawGraph(racketX - playerW / 2, racketY - playerH / 2, imgPlayer, TRUE);

            // ボールとプレイヤーの当たり判定
            dx = ballX - racketX; // x軸方向の距離
            dy = ballY - racketY; // y軸方向の距離
            if (-racketW / 2 - 10 < dx && dx < racketW / 2 + 10 && -20 < dy && dy < 0)
            {
                ballVy = -5 - rand() % 5;
                PlaySoundMem(atack_Se, DX_PLAYTYPE_BACK); 
                if (score > highScore) highScore = score; // ハイスコアの更新

                // === タイミング入力（スペースキー） ===
                if (CheckHitKey(KEY_INPUT_SPACE) == 1) {
                    // スペースが押されていたらボール加速
                    ballVy *= 1.5;   
                    ballVx *= 1.2;
                    PlaySoundMem(jin, DX_PLAYTYPE_BACK); // 成功音を再生（レベルアップ音でもOK）

                    // 加速エフェクト
                    SetDrawBlendMode(DX_BLENDMODE_ADD, 128);
                    DrawCircle(ballX, ballY, 30, GetColor(255, 255, 100), FALSE);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

                }

                prevSpace = nowSpace;

                //レベルアップの処理
                reflectCount++;

                if (reflectCount >= maxReflect) {
                    reflectCount = 0;          // カウントリセット
                    playerLevel++;             // レベルアップ
                    playerAttack++;            // 攻撃力上昇
                    PlaySoundMem(powerup_Se, DX_PLAYTYPE_BACK); // レベルアップ音
                }
            }

            // ==== 敵とボールの当たり判定 ====
            if (ballHitCoolTime == 0) {
                for (int i = 0; i < ENEMY_MAX; i++) {
                    if (enemies[i].alive) {

                        // 敵の座標範囲
                        int left = enemies[i].x;
                        int right = enemies[i].x + enemies[i].w;
                        int top = enemies[i].y;
                        int bottom = enemies[i].y + enemies[i].h;

                        // ボールの中心座標と半径
                        int bx = ballX;
                        int by = ballY;
                        int r = ballR;

                        // ---- 衝突判定（円と矩形の簡易判定）----
                        // ① ボールの中心が敵の四角の中にある
                        if (bx + r > left && bx - r < right &&
                            by + r > top && by - r < bottom)
                        {
                            enemies[i].hp -= playerAttack; //  攻撃力分だけHPを減らす

                            // === ヒット時にエフェクト生成 ===
                            for (int e = 0; e < EFFECT_MAX; e++) {
                                if (!effects[e].active) {
                                    effects[e].active = true;
                                    effects[e].x = (left + right) / 2;
                                    effects[e].y = (top + bottom) / 2;
                                    effects[e].frame = 0;
                                    break;
                                }
                            }

                            // HPが0以下になったら倒す
                            if (enemies[i].hp <= 0) {
                                enemies[i].alive = false;

                                int addScore = 0;
                                switch (enemies[i].sizeType) {
                                case 0: addScore = 100; break; // 小
                                case 1: addScore = 300; break; // 中
                                case 2: addScore = 600; break; // 大
                                }

                                score += addScore;
                            }

                            ballVy = -ballVy;           // ボールを反射
                            PlaySoundMem(atack_Se, DX_PLAYTYPE_BACK); // 効果音
                            ballHitCoolTime = 20;//クールタイム
                            break; // 1体にしか当たらないようにループ終了
                        }
                    }
                }
            }

            for (int i = 0; i < EFFECT_MAX; i++) {
                if (effects[i].active) {
                    effects[i].frame++;

                    // フェードアウトする半径と透明度を計算
                    int r = 10 + effects[i].frame * 2;     // 半径が広がる
                    int alpha = 255 - effects[i].frame * 15; // 徐々に消える
                    if (alpha < 0) alpha = 0;

                    // 光のエフェクトを描画
                    SetDrawBlendMode(DX_BLENDMODE_ADD, alpha); // 加算合成で光る
                    DrawCircle(effects[i].x, effects[i].y, r, GetColor(255, 200, 100), TRUE);
                    DrawCircle(effects[i].x, effects[i].y, r / 2, GetColor(255, 255, 255), TRUE);
                    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

                    // 一定時間で無効化
                    if (effects[i].frame > 15) { // 約0.25秒で消える
                        effects[i].active = false;
                    }
                }
            }

            

            // === 経過時間をカウントする処理 ===
            elapsedFrame++;

            // 残り時間を秒に変換
            remainingSec = LIMIT_TIME - elapsedFrame / 60; // 1秒 = 60フレーム

            // === 時間切れチェック ===
            if (remainingSec <= 0) {
                scene = RESULT_Scene;
                timer = 0;
                StopSoundMem(bgm);
                PlaySoundMem(result, DX_PLAYTYPE_BACK);
            }

            break;

        // リザルト画面の処理
        case RESULT_Scene:
            DrawGraph(0, 0, imgRs, FALSE); // 背景の描画
            SetFontSize(40);
            DrawString(WIDTH / 2 - 150, HEIGHT / 3, "TIME UP!", GetColor(255, 255, 0));
            DrawFormatString(WIDTH / 2 - 100, HEIGHT / 2, GetColor(255, 0, 0), "SCORE: %d", score);
            if (timer > 60 * 5) {
                scene = TITLE_Scene; // 5秒後タイトルに戻る
                StopSoundMem(result);
                PlaySoundMem(titleBgm, DX_PLAYTYPE_LOOP);
            }
            break;

        //  ==== ゲームオーバーの処理 ====
        case GAMEOVER_Scene: 
            DrawGraph(0, 0, imgGo, FALSE); // 背景の描画
            SetFontSize(40);
            DrawString(WIDTH / 2 - 40 / 2 * 9 / 2, HEIGHT / 3, "GAME OVER", 0xff0000);
            if (timer > 60 * 5) {
                scene = TITLE_Scene;
                StopSoundMem(jin);
                PlaySoundMem(titleBgm, DX_PLAYTYPE_LOOP);
            } 
            break;
        }

        ScreenFlip(); // 裏画面の内容を表画面に反映させる
        WaitTimer(16); // 一定時間待つ
        if (ProcessMessage() == -1) break; // Windowsから情報を受け取りエラーが起きたら終了
        if (CheckHitKey(KEY_INPUT_ESCAPE) == 1) break; // ESCキーが押されたら終了
    }

    DxLib_End(); // ＤＸライブラリ使用の終了処理
    return 0; // ソフトの終了
}