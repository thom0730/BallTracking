#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    //ofSetWindowShape(400, 400);
    ofBackground(0, 0, 0);
    ofSetVerticalSync(true);
    ofSetFrameRate(60);
    ofEnableAlphaBlending();
    
    //UDP set up
    if (isBind) { // Bind済みかどうかのbool
        udpConnect.Close();
        udpConnect.Create();
    }
    isBind = udpConnect.Bind(56789); //SDCBallSim's Port
    udpConnect.SetNonBlocking(true);
    
    //GUI set up
    gui.setup();
    gui.add(Threshold.setup("Threshold", -1, -5, 0));
    
    //ボールのバウンド検出用のフラグの初期化
    for(int i = 0 ; i < BALL_NUM; i++){
        flg[i] = false;
    }
    
    // OSC set up
    sender.setup(HOST, PORT);

    ofSetBackgroundAuto(false);

}

//--------------------------------------------------------------
void ofApp::update(){
    udpConnect.Receive((char*)&packet,(int)sizeof(packet));
   //debug(packet);
    
    //ID：1のボールが動画途中でID：3になってしまうので突貫の仕様
    if(packet.ballId == 2){
        int i = 1;
        bp[i] = packet;
        flg[i] = detect(i);
    }else{
        int i = 0;
        bp[i] = packet;
        flg[i] = detect(i);
    }
    //1フレーム前の位置情報をバッファに格納 ・　OSC 送信
    for(int i = 0  ; i < BALL_NUM ; i++){
        sendOSC(bp[i], flg[i], VecSize[i]);
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    // 画面をフェード
    ofSetColor(0, 0, 0, 5);
    ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());
    
    ofSetColor(255, 255, 255);
    if(packet.flag == 1){
        //文字表示画面の背景
        ofSetColor(0, 0, 0);
        ofDrawRectangle(0, 0, ofGetWidth(),100);
        for(int i = 0 ; i < 2 ; i++){
            //動画サイズ(フルHD)をWindow Sizeに変換
            float x = ofMap(bp[i].x,0,1920,0,ofGetWidth());
            float y = ofMap(bp[i].y,0,1080,0,ofGetHeight());
            
            //display
            ofSetColor(255, 255, 255);
            ofDrawBitmapString( "Ball ID = " + ofToString(bp[i].ballId) , bp[i].ballId * 300, 20);
            ofDrawBitmapString( "x = " + ofToString(x) , bp[i].ballId * 300, 40);
            ofDrawBitmapString( "y = " + ofToString(y) , bp[i].ballId * 300, 60);
            if(flg){
                ofDrawBitmapString( "detect " ,1000, 60);
            }
            
            //Ball
           // ofDrawBitmapString( "Ball ID = " + ofToString(bp[i].ballId) , x, y);
            ofDrawCircle(x,y, 5);
        }
    }
    //方眼紙
    drawGrid();
    // GUIを表示
    gui.draw();

}
//--------------------------------------------------------------
bool ofApp::detect(int _i){
    vec[_i].set(bp[_i].x - buffx[_i] , bp[_i].y - buffy[_i]);
    float x = vec[_i].x * buffvec[_i].x;
    float y = vec[_i].y * buffvec[_i].y;
    bool flg = false;
    if(y < Threshold){
        flg = true;
        VecSize[_i] = pow((vec[_i].x - buffvec[_i].x),2) + pow((vec[_i].y - buffvec[_i].y),2);
        cout << VecSize[_i] << endl;
    }
    buffx[_i] = bp[_i].x;
    buffy[_i] = bp[_i].y;
    buffvec[_i] = vec[_i];
    
    return flg;
}
//--------------------------------------------------------------
void ofApp::sendOSC(BallPacket _bp, bool _flg, float _attack){
    float attack = 0;
    int note = 0;
    //ボールのバウンドが検出
    if(_flg){
        attack = _attack;
        if(_bp.y < ofGetHeight()/3){
            note = 1;
        }else if(_bp.y > ofGetHeight()/3 &&  _bp.y < 2*ofGetHeight()/3){
            note = 2;
        }else{
            note = 3;
        }
    }
    //IDを整理
    int number;
    if(_bp.ballId == 2){
        number = 2;
    }else{
        number = 1;
    }
    
    for(int i = 0 ; i < 4 ; i++){
        //OSCメッセージの準備
        ofxOscMessage m;
        string msg = "";
        switch (i) {
            case 0:
                msg += "/Ball" + ofToString(number) + "_x";
                m.setAddress(msg);
                m.addFloatArg(_bp.x);
                break;
            case 1:
                msg += "/Ball" + ofToString(number) + "_y";
                m.setAddress(msg);
                m.addFloatArg(_bp.y);
                break;
            case 2:
                msg += "/Ball" + ofToString(number) + "_attack";
                m.setAddress(msg);
                m.addFloatArg(attack);
                break;
            case 3:
                msg += "/Ball" + ofToString(number) + "_note";
                m.setAddress(msg);
                m.addIntArg(note*2);
                break;
            default:
                break;
        }
        sender.sendMessage(m);
    }
}
//--------------------------------------------------------------
void ofApp::drawGrid(){
    
    ofPushMatrix();
    ofTranslate(ofGetWidth()/2, ofGetHeight()/2);
    int lines = ofGetWidth()/2/20+1;
    ofSetColor(70, 70, 70);
    for (int i=0; i < lines; i++) {
        ofSetColor(255, 255, 255,2);
        ofDrawLine(20*i, -1*ofGetHeight()/2, 20*i, ofGetHeight()/2);
        ofDrawLine(-20*i, -1*ofGetHeight()/2, -20*i, ofGetHeight()/2);
        ofDrawLine(-1*ofGetWidth()/2, 20*i, ofGetWidth()/2, 20*i);
        ofDrawLine(-1*ofGetWidth()/2, -20*i, ofGetWidth()/2, -20*i);
    }
    ofPopMatrix();
    for(int i = 0 ; i < ofGetHeight() ; i ++){
        if((20*i)%100 == 0){
            ofSetColor(255, 0, 0 );
            ofDrawBitmapString( ofToString(i*20) , 10, 20*i);
        }
    }
}
//--------------------------------------------------------------
void ofApp::debug(BallPacket _bp){
    cout << "＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝" << endl;
    cout <<  "header = " << _bp.header << endl;
    cout <<  "index = " << _bp.index << endl;
    cout <<  "ballId = " << _bp.ballId << endl;
    cout <<  "flag = " << _bp.flag << endl;
    cout <<  "timestamp = " << _bp.timestamp << endl;
    cout <<  "x = " << _bp.x << endl;
    cout <<  "y = " << _bp.y << endl;
}
//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
//--------------------------------------------------------------
void ofApp::exit(){
    if(isBind) udpConnect.Close();
}
