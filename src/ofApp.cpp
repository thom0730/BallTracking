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
    
    // OSC set up
    sender.setup(HOST, PORT);

    //軌跡が残るように
    ofSetBackgroundAuto(false);

}

//--------------------------------------------------------------
void ofApp::update(){
    //UDP受信
    udpConnect.Receive((char*)&packet,(int)sizeof(packet));
   //debug(packet);
    /*
     ID：1のボールが動画途中でID：3になってしまうので突貫の仕様
     Ball ID 1,3：bp[0]
     Ball ID 2：bp[1]
     */
    //IDを整理
    int number;
    if(packet.ballId % 2 == 0){
        number = 1;
    }else{
        number = 0;
    }
    bp[number] = packet;
    detect(number);

    //OSC 送信
    for(int i = 0  ; i < BALL_NUM ; i++){
        soundCreate(i);
        sendOSC(bp[i], i);
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
            ofDrawBitmapString( "Ball ID = " + ofToString(i+1) , (i+1) * 300, 20);
            ofDrawBitmapString( "x = " + ofToString(x) , (i+1) * 300, 40);
            ofDrawBitmapString( "y = " + ofToString(y) , (i+1) * 300, 60);
            //Ball
            ofDrawBitmapString( "attack = " + ofToString(VecSize[i]) , (i+1) * 300, 80);
            ofDrawBitmapString( "note = " + ofToString(note[i]) , (i+1) * 300, 100);
            ofSetColor((i+1)*100, 255, 255);
            ofDrawCircle(x,y, 5);
        }
    }
    //方眼紙
    drawGrid();
    // GUIを表示
    gui.draw();

}
//--------------------------------------------------------------
void ofApp::detect(int _i){
    //前のフレームの座標->現在座標のベクトルを生成
    vec[_i].set(bp[_i].x - buffx[_i] , bp[_i].y - buffy[_i]);
    //前フレームで生成したベクトルと現在ベクトルの積が負になれば速度ベクトルが逆転していると判定
    float x = vec[_i].x * buffvec[_i].x;
    float y = vec[_i].y * buffvec[_i].y;
    
    VecSize[_i] = 0;
    note[_i] = 0;
    
    if(y < Threshold){
        //現在ベクトルの大きさをattackとして出力
        VecSize[_i] = ABS(bp[_i].y - buffy[_i]);
        
        //ボールが消えた時のattackの検出を外す
        if(VecSize[_i] > 400 ){
            VecSize[_i]  = 0;
        }
    }
    //現在座標をバッファに格納し、次フレームでのベクトル生成に使用
    buffx[_i] = bp[_i].x;
    buffy[_i] = bp[_i].y;
    buffvec[_i] = vec[_i];
}
//--------------------------------------------------------------
void ofApp::sendOSC(BallPacket _bp, int _i){
    //IDを整理(配列とは関係ない)
    int BallID;
    if(_bp.ballId % 2 == 0){
        BallID = 2;
    }else{
        BallID = 1;
    }
    for(int i = 0 ; i < 4 ; i++){
        //OSCメッセージの準備
        ofxOscMessage m;
        string msg = "";
        switch (i) {
            case 0:
                msg += "/Ball" + ofToString(BallID) + "_x";
                m.setAddress(msg);
                m.addFloatArg(_bp.x);
                break;
            case 1:
                msg += "/Ball" + ofToString(BallID) + "_y";
                m.setAddress(msg);
                m.addFloatArg(_bp.y);
                break;
            case 2:
                msg += "/Ball" + ofToString(BallID) + "_attack";
                m.setAddress(msg);
                m.addFloatArg(VecSize[_i]);
                break;
            case 3:
                msg += "/Ball" + ofToString(BallID) + "_note";
                m.setAddress(msg);
                m.addIntArg(note[_i]);
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
void ofApp::soundCreate(int _i){
    //音色分け
    if(bp[_i].y < ofGetHeight()/3){
        note[_i] = 1 * (_i+1);
    }else if(bp[_i].y > ofGetHeight()/3 &&  bp[_i].y < 2*ofGetHeight()/3){
        note[_i] = 2 * (_i+1);
    }else{
        note[_i] = 3 * (_i+1);
    }
    
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
