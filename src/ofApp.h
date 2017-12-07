#pragma once

#include "ofMain.h"
#include "ofxNetwork.h"
#include "ofxOsc.h"
#include "ofxGui.h"
#include "drawData.hpp"
#include "ofxCv.h"

#define BALL_NUM 2
//リアルタイム：SAMPLE_RATE = 4 | 録画：SAMPLE_RATE = 2
#define SAMPLE_RATE 2
#define HOST "127.0.0.1" // IPアドレス
#define PORT 8000 // ポート番

//ボール番号の割り振り
#define LEFT 1
#define RIGHT 2

struct BallPacket{
    uint32_t header;
    uint32_t index;
    uint32_t ballId;
    uint32_t flag;
    float timestamp;
    float x, y, z;
};

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();
        void exit();
		void keyPressed(int key);

    //CORE
    void buffering(BallPacket _bp, int _i);
    void detect(BallPacket _bp, int _i);
    void mainSoundCreate(BallPacket _bp, int _i);
    void sendOSC(BallPacket _bp, int _i);
    
    //FOR DEBUG・DRAW
    void debug(BallPacket _bp);
    void trackingDraw();
    void graphDraw();
    int countFrame;

    //FOR PRESET
    int fullHD_x = 1920;
    int fullHD_y = 1080;

    //FOR RECEIVE UDP
    ofxUDPManager udpConnect;
    float mx,my;
    bool isBind = true;
    
    //FOR PROCESSING
    BallPacket packet; //受信用
    BallPacket bp[BALL_NUM];
    float Lprev_x[SAMPLE_RATE] , Lprev_y[SAMPLE_RATE];
    float Rprev_x[SAMPLE_RATE] , Rprev_y[SAMPLE_RATE];
    ofVec2f Lprev_vec, Rprev_vec; //1フレーム前の速度ベクトル
    ofVec2f L_vec, R_vec; //現在の速度ベクトル
    int buffArrID; //ボールが消えたときのIDの整理用
    float attack[BALL_NUM]; //Attackデータ
    int note[BALL_NUM]; // 音色データ
    
    //FOR KALMAN FILTER
    ofxCv::KalmanPosition kalman[BALL_NUM];
    ofVec2f pre_pos[BALL_NUM],est_pos[BALL_NUM];
    ofMesh predicted[BALL_NUM], line[BALL_NUM], estimated[BALL_NUM];
    float speed[BALL_NUM];

    //FOR GUI
    ofxPanel gui;
    ofxFloatSlider Threshold;
    ofxFloatSlider DetectMIN;
    ofxFloatSlider DetectMAX;
    ofxIntSlider SampleRate;
    float initThres = -1.0;
    float initMax = 200.0;
    float initMin = 1.0;
    int initSample = 4;
    
    //FOR OSC
    ofxOscSender sender;
    
    //FOR TIMELINE
    void introFLG();
    bool flg = false;
    
    drawData drawdata;
    

};
