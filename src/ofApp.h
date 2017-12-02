#pragma once

#include "ofMain.h"
#include "ofxNetwork.h"
#include "ofxOsc.h"
#include "ofxGui.h"
#include "drawData.hpp"

#define BALL_NUM 2
#define SAMPLE_RATE 4
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
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
    
    //CORE
    void buffering(BallPacket _bp, int _i);
    void detect(BallPacket _bp, int _i);
    void mainSoundCreate(BallPacket _bp, int _i);
    void sendOSC(BallPacket _bp, int _i);
    
    //FOR DEBUG・DRAW
    void debug(BallPacket _bp);
    void trackingDraw();
    void graphDraw();
    
    void lowpassFilter(float _posPrev, float _posNew);
    float alpha = 0.8;


    //FOR PRESET
    int fullHD_x = 1920;
    int fullHD_y = 1080;
    float BPM = 100.0;

    ofxUDPManager udpConnect;
    float mx,my;
    bool isBind = true;
    
    //FOR PROCESSING
    BallPacket packet; //受信用
    BallPacket bp[BALL_NUM];
    vector<BallPacket> ballpacket; //受信用(遅延する場合)
    float Lprev_x[SAMPLE_RATE] , Lprev_y[SAMPLE_RATE];
    float Rprev_x[SAMPLE_RATE] , Rprev_y[SAMPLE_RATE];
    ofVec2f Lprev_vec, Rprev_vec; //1フレーム前の速度ベクトル
    ofVec2f L_vec, R_vec; //現在の速度ベクトル
    int buffArrID; //ボールが消えたときのIDの整理用
    float attack[BALL_NUM]; //Attackデータ
    int note[BALL_NUM]; // 音色データ
    
    
    
    //以下いらないかも
    void detect1(int _i);
    void detect2(int _i);
    void detect3(int _i);
    float buffx[BALL_NUM], buffy[BALL_NUM]; //1フレーム前の位置
    ofVec3f vec[BALL_NUM];//現在のベクトル
    ofVec3f buffvec[BALL_NUM]; //1フレーム前のベクトル
    float bbuffx[BALL_NUM], bbuffy[BALL_NUM]; //1フレーム前の位置
    ofVec3f bbuffvec[BALL_NUM]; //1フレーム前のベクトル
    int countFrame;
    
    
    
    //GUI
    ofxPanel gui;
    ofxFloatSlider Threshold;
    //OSCメッセージの送信者
    ofxOscSender sender;
    
    drawData drawdata;
    

};
