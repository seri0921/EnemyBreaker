#include "DxLib.h"
#include <stdlib.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    const int WIDTH = 960, HEIGHT = 640; // ウィンドウの幅と高さのピクセル数

    SetWindowText("テニスゲーム"); // ウィンドウのタイトル
    SetGraphMode(WIDTH, HEIGHT, 32); // ウィンドウの大きさとカラービット数の指定
    ChangeWindowMode(TRUE); // ウィンドウモードで起動
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

    while (1) // メインループ
    {
        ClearDrawScreen(); // 画面をクリアする
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
                scene = PLAY;
            }
            break;

        case PLAY: // ゲームをプレイする処理
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
                break;
            }
            DrawCircle(ballX, ballY, ballR, 0xff0000, TRUE); // ボール

            // ラケットの処理
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
            DrawBox(racketX - racketW / 2, racketY - racketH / 2, racketX + racketW / 2, racketY + racketH / 2, 0x0080ff, TRUE); // ラケット

            // ヒットチェック
            dx = ballX - racketX; // x軸方向の距離
            dy = ballY - racketY; // y軸方向の距離
            if (-racketW / 2 - 10 < dx && dx < racketW / 2 + 10 && -20 < dy && dy < 0)
            {
                ballVy = -5 - rand() % 5;
                score = score + 100;
                if (score > highScore) highScore = score; // ハイスコアの更新
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