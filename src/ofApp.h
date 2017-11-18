#pragma once

#include "ofMain.h"
#include "ofxNetwork.h"
#include "ofxOsc.h"
#include "ofxGui.h"

#define BALL_NUM 2

#define HOST "127.0.0.1" // 受信側のIPアドレス
#define PORT 8000 // 受信側のポート番号

    
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
    void sendOSC(BallPacket _bp, bool _flg, float _attack);
    bool detect(int _i);
    void drawGrid();

    ofxUDPManager udpConnect;
    float mx,my;
    bool isBind = true;
    BallPacket packet; //受信用
    BallPacket bp[BALL_NUM];
    float buffx[BALL_NUM], buffy[BALL_NUM]; //1フレーム前の位置
    ofVec3f vec[BALL_NUM];//現在のベクトル
    ofVec3f buffvec[BALL_NUM]; //1フレーム前のベクトル
    bool flg[BALL_NUM] ; //ボールのバウンドの検出判定用
    float VecSize[BALL_NUM];
    
    //GUI
    ofxPanel gui;
    ofxFloatSlider Threshold;
    //OSCメッセージの送信者
    ofxOscSender sender;
};
