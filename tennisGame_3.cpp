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

    while (1) // メインループ
    {
        ClearDrawScreen(); // 画面をクリアする
        // ボールの処理
        ballX = ballX + ballVx;
        if (ballX < ballR && ballVx < 0) ballVx = -ballVx;
        if (ballX > WIDTH - ballR && ballVx > 0) ballVx = -ballVx;
        ballY = ballY + ballVy;
        if (ballY < ballR && ballVy < 0) ballVy = -ballVy;
        if (ballY > HEIGHT && ballVy > 0) ballVy = -ballVy;
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
        int dx = ballX - racketX; // x軸方向の距離
        int dy = ballY - racketY; // y軸方向の距離
        if (-racketW / 2 - 10 < dx && dx < racketW / 2 + 10 && -20 < dy && dy < 0) ballVy = -5 - rand() % 5;

        ScreenFlip(); // 裏画面の内容を表画面に反映させる
        WaitTimer(16); // 一定時間待つ
        if (ProcessMessage() == -1) break; // Windowsから情報を受け取りエラーが起きたら終了
        if (CheckHitKey(KEY_INPUT_ESCAPE) == 1) break; // ESCキーが押されたら終了
    }

    DxLib_End(); // ＤＸライブラリ使用の終了処理
    return 0; // ソフトの終了
}