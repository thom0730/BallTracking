#pragma once

#include "ofMain.h"
#include "ofxNetwork.h"
#include "ofxOsc.h"
#include "ofxGui.h"
#include "drawData.hpp"

#define BALL_NUM 2
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
    
    void debug(BallPacket _bp);
    void trackingDraw();
    void graphDraw();
    void sendOSC(BallPacket _bp, int _i);
    void detect(int _i);
    void detect2(int _i);
    void detect3(int _i);
    void mainSoundCreate(int _i);
    void lowpassFilter(float _posPrev, float _posNew);
    float alpha = 0.8;


    int fullHD_x = 1920;
    int fullHD_y = 1080;
    float BPM = 100.0;
    int buffArrID; //ボールが消えたときのIDの整理用
    int countFrame;

    ofxUDPManager udpConnect;
    float mx,my;
    bool isBind = true;
    
    BallPacket packet; //受信用
    BallPacket bp[BALL_NUM];
    float buffx[BALL_NUM], buffy[BALL_NUM]; //1フレーム前の位置
    ofVec3f vec[BALL_NUM];//現在のベクトル
    ofVec3f buffvec[BALL_NUM]; //1フレーム前のベクトル
    float bbuffx[BALL_NUM], bbuffy[BALL_NUM]; //1フレーム前の位置
    ofVec3f bbuffvec[BALL_NUM]; //1フレーム前のベクトル
    float attack[BALL_NUM]; //Attackデータ
    int note[BALL_NUM]; // 音色データ
    
    //GUI
    ofxPanel gui;
    ofxFloatSlider Threshold;
    //OSCメッセージの送信者
    ofxOscSender sender;
    
    drawData drawdata;
    

};
