#include "DxLib.h"
#include <stdlib.h>
#include <time.h>

const int WIDTH = 960, HEIGHT = 640; // ウィンドウの幅と高さのピクセル数

// 敵に関する情報
struct Enemy {
    int x;
    int y;
    int w;
    int h;
    int vy; //下に降りてくる速度
    int hp; //敵の体力
    int moveTimer; //下に降りてくる時間間隔
    bool alive;
};

const int ENEMY_MAX = 50; //全体の敵数
const int ENEMY_BATCH = 5; // 一度に出す数

Enemy enemies[ENEMY_MAX]; //エネミー構造体の配列

int enemySpawnTimer = 0;   // 敵生成用タイマー
const int ENEMY_SPAWN_INTERVAL = 6000; //生成間隔、60につき１秒

//最初に生成するための変数
bool firstSpawn = true;

//敵が重ならないように初期化するための関数
void EnemySpawn() {
    srand((unsigned int)time(NULL));

    for (int i = 0; i < ENEMY_MAX; i++) {
        enemies[i].w = 60;
        enemies[i].h = 20;
        enemies[i].alive = true;
        enemies[i].vy = 7;
        enemies[i].moveTimer = 0;
        enemies[i].hp = 3;

        bool overlap;
        do {
            overlap = false;

            enemies[i].x = rand() % (WIDTH - enemies[i].w);
            enemies[i].y = rand() % 80 + 20;

            for (int j = 0; j < i; j++) {
                int dx = abs(enemies[i].x - enemies[j].x);
                int dy = abs(enemies[i].y - enemies[j].y);
                if (dx < enemies[i].w + 20 && dy < enemies[i].h + 20) {
                    overlap = true;
                    break;
                }
            }
        } while (overlap);
    }
}

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
    int racketW = 120;
    int racketH = 12;



    // ゲーム進行に関する変数、スコアを代入する変数
    enum { TITLE, PLAY, OVER };
    int scene = TITLE;
    int timer = 0;
    int score = 0; // スコアを代入
    int highScore = 1000; // ハイスコアを代入
    int dx, dy; // ヒットチェック用の変数宣言

    int imgBg = LoadGraph("image/bg.png"); // 背景画像の読み込み

    // サウンドの読み込みと音量設定
    int bgm = LoadSoundMem("sound/bgm.mp3");
    int jin = LoadSoundMem("sound/gameover.mp3");
    int se = LoadSoundMem("sound/hit.mp3");
    ChangeVolumeSoundMem(128, bgm);
    ChangeVolumeSoundMem(128, jin);

    while (1) // メインループ
    {
        ClearDrawScreen(); // 画面をクリアする
        DrawGraph(0, 0, imgBg, FALSE); // 背景の描画
        timer++;

        switch (scene) // タイトル、ゲームをプレイ、ゲームオーバーの分岐
        {
        case TITLE: // タイトル画面の処理
            SetFontSize(50);
            DrawString(WIDTH / 2 - 50 / 2 * 12 / 2, HEIGHT / 3, "Tennis Game", 0x00ff00);
            if (timer % 60 < 30) { // 文字を点滅表示
                SetFontSize(30);
                DrawString(WIDTH / 2 - 30 / 2 * 21 / 2, HEIGHT * 2 / 3, "Press SPACE to start.", 0x00ffff);
            }
            if (CheckHitKey(KEY_INPUT_SPACE) == 1) // スペースキー押し下し
            {
                ballX = 40;
                ballY = 80;
                ballVx = 5;
                ballVy = 5;
                racketX = WIDTH / 2;
                racketY = HEIGHT - 50;
                score = 0;

                scene = PLAY; //プレイ画面へ移動
                PlaySoundMem(bgm, DX_PLAYTYPE_LOOP); // BGMをループ再生
            }
            break;

        case PLAY: // ゲームをプレイする処理
            //最初の5体を出す
            if (firstSpawn) {
                srand((unsigned int)time(NULL));

                for (int i = 0; i < ENEMY_BATCH; i++) {

                    enemies[i].w = 60;
                    enemies[i].h = 20;
                    enemies[i].vy = 1;
                    enemies[i].hp = 3;
                    enemies[i].x = rand() % (WIDTH - 60);
                    enemies[i].y = 20 + rand() % 60;
                    //重なりがないかチェック！
                    bool overlap;
                    do {
                        overlap = false;

                        enemies[i].x = rand() % (WIDTH - enemies[i].w);
                        enemies[i].y = rand() % 80 + 20;

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
                firstSpawn = false;
            }

            // ボールの処理
            ballX = ballX + ballVx;
            if (ballX < ballR && ballVx < 0) ballVx = -ballVx;
            if (ballX > WIDTH - ballR && ballVx > 0) ballVx = -ballVx;
            ballY = ballY + ballVy;
            if (ballY < ballR && ballVy < 0) ballVy = -ballVy;
            //            if (ballY > HEIGHT && ballVy > 0) ballVy = -ballVy;
            if (ballY > HEIGHT) // ボールが下端に達した時
            {
                scene = OVER;
                timer = 0;
                StopSoundMem(bgm); // BGMを停止
                PlaySoundMem(jin, DX_PLAYTYPE_BACK); // ジングルを出力
                break;
            }
            DrawCircle(ballX, ballY, ballR, 0xff0000, TRUE); // ボール
            DrawCircle(ballX - ballR / 4, ballY - ballR / 4, ballR / 2, 0xffa0a0, TRUE);
            DrawCircle(ballX - ballR / 4, ballY - ballR / 4, ballR / 4, 0xffffff, TRUE);

            // ==== 敵生成処理 ====
            enemySpawnTimer++;

            if (enemySpawnTimer >= ENEMY_SPAWN_INTERVAL) {
                enemySpawnTimer = 0; // タイマーリセット

                int spawned = 0; // この周期で何体出したか

                // 空いてるスロットを探して最大5体まで生成
                for (int i = 0; i < ENEMY_MAX && spawned < ENEMY_BATCH; i++) {
                    if (!enemies[i].alive) {

                        srand((unsigned int)time(NULL));

                        enemies[i].w = 60;
                        enemies[i].h = 20;
                        enemies[i].vy = 3;
                        enemies[i].moveTimer = 0;
                        enemies[i].hp = 3;
                        //重ならないようにする処理
                        bool overlap;
                        do {
                            overlap = false;

                            enemies[i].x = rand() % (WIDTH - enemies[i].w);
                            enemies[i].y = rand() % 80 + 20;

                            for (int j = 0; j < i; j++) {
                                int dx = abs(enemies[i].x - enemies[j].x);
                                int dy = abs(enemies[i].y - enemies[j].y);
                                if (dx < enemies[i].w + 20 && dy < enemies[i].h + 20) {
                                    overlap = true;
                                    break;
                                }
                            }
                        } while (overlap);
                        enemies[i].alive = true;
                        spawned++;
                    }
                }
            }


            //敵の移動処理
            for (int i = 0; i < ENEMY_MAX; i++) {
                if (enemies[i].alive) {
                    enemies[i].moveTimer++;

                    // 一定時間ごとにゆっくり下降
                    if (enemies[i].moveTimer >= 60) { // 30フレーム＝約0.5秒
                        enemies[i].y += enemies[i].vy;
                        enemies[i].moveTimer = 0;
                    }

                    // 描画
                    DrawBox(
                        enemies[i].x, enemies[i].y,
                        enemies[i].x + enemies[i].w,
                        enemies[i].y + enemies[i].h,
                        GetColor(255, 0, 0), TRUE
                    );

                    // 画面下に出たら消す
                    if (enemies[i].y > HEIGHT) {
                        enemies[i].alive = false;
                    }
                }
            }


            // プレイヤーの処理
            if (CheckHitKey(KEY_INPUT_LEFT) == 1) // 左キー押し下し
            {
                racketX = racketX - 10;
                if (racketX < racketW / 2) racketX = racketW / 2;
            }
            if (CheckHitKey(KEY_INPUT_RIGHT) == 1) // 右キー押し下し
            {
                racketX = racketX + 10;
                if (racketX > WIDTH - racketW / 2) racketX = WIDTH - racketW / 2;
            }
            DrawBox(racketX - racketW / 2 - 2, racketY - racketH / 2 - 2, racketX + racketW / 2, racketY + racketH / 2, 0x40c0ff, TRUE);
            DrawBox(racketX - racketW / 2, racketY - racketH / 2, racketX + racketW / 2 + 2, racketY + racketH / 2 + 2, 0x204080, TRUE);
            DrawBox(racketX - racketW / 2, racketY - racketH / 2, racketX + racketW / 2, racketY + racketH / 2, 0x0080ff, TRUE); // ラケット

            // ヒットチェック
            dx = ballX - racketX; // x軸方向の距離
            dy = ballY - racketY; // y軸方向の距離
            if (-racketW / 2 - 10 < dx && dx < racketW / 2 + 10 && -20 < dy && dy < 0)
            {
                ballVy = -5 - rand() % 5;
                score = score + 100;
                if (score > highScore) highScore = score; // ハイスコアの更新
                PlaySoundMem(se, DX_PLAYTYPE_BACK); // 効果音を出力
            }

            // ==== 敵との当たり判定 ====
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
                        enemies[i].hp--; //敵のHPを減らす
                        // HPが0以下になったら倒す
                        if (enemies[i].hp <= 0) {
                            enemies[i].alive = false;
                            score += 100;
                        }
                        else {
                            score += 30; // 少しだけ加点（ダメージ時）
                        }

                        ballVy = -ballVy;           // ボールを反射
                        PlaySoundMem(se, DX_PLAYTYPE_BACK); // 効果音
                        break; // 1体にしか当たらないようにループ終了
                    }
                }
            }

            break;

        case OVER: // ゲームオーバーの処理
            SetFontSize(40);
            DrawString(WIDTH / 2 - 40 / 2 * 9 / 2, HEIGHT / 3, "GAME OVER", 0xff0000);
            if (timer > 60 * 5) scene = TITLE;
            break;
        }

        SetFontSize(30); // スコアとハイスコアの文字の大きさ
        DrawFormatString(10, 10, 0xffffff, "SCORE %d", score);
        DrawFormatString(WIDTH - 200, 10, 0xffff00, "HI-SC %d", highScore);

        ScreenFlip(); // 裏画面の内容を表画面に反映させる
        WaitTimer(16); // 一定時間待つ
        if (ProcessMessage() == -1) break; // Windowsから情報を受け取りエラーが起きたら終了
        if (CheckHitKey(KEY_INPUT_ESCAPE) == 1) break; // ESCキーが押されたら終了
    }

    DxLib_End(); // ＤＸライブラリ使用の終了処理
    return 0; // ソフトの終了
}