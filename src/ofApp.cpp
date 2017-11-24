#include "ofApp.h"


//--------------------------------------------------------------
void ofApp::setup(){
    //ofSetWindowShape(400, 400);
    ofBackground(0, 0, 0);
    ofSetVerticalSync(true);
    ofSetFrameRate(60);
    ofEnableAlphaBlending();
    ofSetPolyMode(OF_POLY_WINDING_NONZERO); // this is the normal mode
    
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
    
    //BPMを秒に直す
    BPM = (60/BPM)*4*10;
    
    //初期のNoteを定義
    for(int i = 0  ; i < BALL_NUM ; i++){
        note[i] = i+1;
    }
}

//--------------------------------------------------------------
void ofApp::update(){
    countFrame++;
    //UDP受信
    udpConnect.Receive((char*)&packet,(int)sizeof(packet));
   //debug(packet);

    /*============================
     画面左：bp[0] BALL1 (LEFT = 1)
     画面右：bp[1] BALL2 (RIGHT = 2)
     ============================*/

     //IDを整理
     int number = 0;
     if(packet.x < fullHD_x/2){
         number = LEFT-1;
     }else{
         number = RIGHT-1;
     }
    //(0,0)を受信した時のIDの振り分け
    if(packet.ballId == 0){
        switch(buffArrID){
            case 0:
                number = 1;
                break;
            case 1:
                number = 0;
                break;
        }
    }
    buffArrID = number; //前フレームに受けたボールのID
    
    
    //配列に受信した構造体を格納
    bp[number] = packet;
    //衝突検出(attackの検出)
    detect2(number);
    
    //OSC 送信
    for(int i = 0  ; i < BALL_NUM ; i++){
        mainSoundCreate(i);
        sendOSC(bp[i], i);
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    // 画面をフェード
    ofSetColor(0, 0, 0, 5);
  //  ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());

   // trackingDraw();
    graphDraw();

    //方眼紙
    drawdata.drawGrid();
    // GUIを表示
    gui.draw();
    

}
//--------------------------------------------------------------
void ofApp::detect(int _i){
    attack[_i] = 0;
    
    if(countFrame%SAMPLE_RATE == 0){
        //ローパスフィルタ
        lowpassFilter(buffx[_i],bp[_i].x);
        lowpassFilter(buffy[_i],bp[_i].y);
        lowpassFilter(buffx[_i],bp[_i].x);
        lowpassFilter(buffy[_i],bp[_i].y);
        //前のフレームの座標->現在座標のベクトルを生成
        vec[_i].set(bp[_i].x - buffx[_i] , bp[_i].y - buffy[_i]);

        //前フレームで生成したベクトルと現在ベクトルの積が負になれば速度ベクトルが逆転していると判定
        if(vec[_i].dot(buffvec[_i])<0 && vec[_i].y > 0){

            //現在ベクトルの大きさをattackとして出力
            attack[_i] = ABS(bp[_i].y - buffy[_i]);
            //ボールが消えた時のattackの検出を外す
            if(attack[_i] > 400 ){
                attack[_i]  = 0;
            }
        }
        //現在座標をバッファに格納し、次フレームでのベクトル生成に使用
        buffx[_i] = bp[_i].x;
        buffy[_i] = bp[_i].y;
        buffvec[_i] = vec[_i];
    }
    
}
//--------------------------------------------------------------
void ofApp::detect2(int _i){
    //ローパスフィルタ
    lowpassFilter(buffx[_i],bp[_i].x);
    lowpassFilter(buffy[_i],bp[_i].y);
    //前のフレームの座標->現在座標の速度ベクトルを生成
    vec[_i].set(bp[_i].x - buffx[_i] , bp[_i].y - buffy[_i]);
    //前フレームで生成したベクトルと現在ベクトルの積が負になれば速度ベクトルが逆転していると判定
    float x = vec[_i].x * buffvec[_i].x;
    float y = vec[_i].y * buffvec[_i].y;
    
    attack[_i] = 0;
    
    if(y < Threshold && vec[_i].y < 0){
        //現在ベクトルの大きさをattackとして出力
        attack[_i] = ABS(bp[_i].y - buffy[_i]);
        
        //ボールが消えた時のattackの検出を外す
        if(attack[_i] > 400 ){
            attack[_i]  = 0;
        }else if(attack[_i] > 1.0 && attack[_i] < 10.0){
           // attack[_i] = 10.0;
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
    if(_bp.x < fullHD_x/2){
        BallID = LEFT;
    }else{
        BallID = RIGHT;
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
                m.addFloatArg(attack[_i]);
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
void ofApp::trackingDraw(){
    //文字表示画面の背景
    ofSetColor(0, 0, 0);
    ofDrawRectangle(0, 0, ofGetWidth(),100);
    ofDrawBitmapString(ofToString(ofGetFrameRate())+ "fps" , ofGetWidth() - 100, 20);
    
    for(int i = 0 ; i < BALL_NUM ; i++){
        ofSetColor(255, 255, 255);
        //動画サイズ(フルHD)をWindow Sizeに変換
        float x = ofMap(bp[i].x,0,fullHD_x,0,ofGetWidth());
        float y = ofMap(bp[i].y,0,fullHD_y,0,ofGetHeight());
        
        int j;
        if(i == 0){ //LEFT
            j = 1;
            ofDrawBitmapString( "Ball ID = " + ofToString(LEFT) , (j+1)*300, 20);
        }else{ //RIGHT
            j = 0 ;
            ofDrawBitmapString( "Ball ID = " + ofToString(RIGHT) , (j+1)*300, 20);
        }
        ofDrawBitmapString( "x = " + ofToString(x) , (j+1)*300, 40);
        ofDrawBitmapString( "y = " + ofToString(y) , (j+1)*300, 60);
        ofDrawBitmapString( "attack = " + ofToString(attack[i]) , (j+1)*300, 80);
        ofDrawBitmapString( "note = " + ofToString(note[i]) , (j+1)*300, 100);
        
        //ball
        ofSetColor(i*100 + ofMap(attack[i],0,1500,0,255), 255, 255);
        ofDrawBitmapString( "ID = " + ofToString(i), x+5,y+5);
        ofDrawCircle(x,y, 1);
        
        for(int i = 0 ; i < ofGetHeight() ; i ++){
            if((20*i)%100 == 0){
                ofSetColor(255, 0, 0 );
                ofDrawBitmapString( ofToString(i*20) , 10, 20*i);
            }
        }
    }

}
//--------------------------------------------------------------
void ofApp::graphDraw(){
    for(int i = 1 ; i < 2 ; i++){
        ofSetColor(255, 255, 255);
        //動画サイズ(フルHD)をWindow Sizeに変換
        float x = ofMap(bp[i].x,0,fullHD_x,0,ofGetWidth());
        float y = ofMap(bp[i].y,0,fullHD_y,0,ofGetHeight());
        
        int j = 0;
        if(attack[i] != 0){
            j = 100;
            ofDrawBitmapString( "ID = " + ofToString(i), 10+ofMap(countFrame,0,2000,0,ofGetWidth()),y+5);
            ofDrawBitmapString( "Attack = " + ofToString(attack[i]), 10+ofMap(countFrame,0,2000,0,ofGetWidth()),y+15);
        }
        //ball
        ofBeginShape();
        ofSetColor(i*100 + j, 100*(i+1), 155 + j);
        //ofDrawBitmapString( "ID = " + ofToString(i), x+5,y+5);
        ofVertex(ofMap(countFrame,0,2000,0,ofGetWidth()),y);
        ofDrawCircle(ofMap(countFrame,0,2000,0,ofGetWidth()),y, 1 + (j/20));
        ofEndShape();
        
        //座標の描画
        for(int i = 0 ; i < ofGetHeight() ; i ++){
            if((20*i)%100 == 0){
                ofSetColor(255, 0, 0 );
                ofDrawBitmapString( ofToString(ofGetHeight() - i*20) , 10, 20*i);
            }
        }
    }
    
}
//--------------------------------------------------------------
void ofApp::lowpassFilter(float _posPrev, float _posNew){
    _posPrev = alpha * _posPrev + (1-alpha) * _posNew;

}
//--------------------------------------------------------------
void ofApp::debug(BallPacket _bp){
    cout << "＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝" << endl;
    cout <<  "header = " << _bp.header << endl;
    cout <<  "index = " << _bp.index << endl;
    cout <<  "ballId = " << _bp.ballId << endl;
    cout <<  "flag = " << _bp.flag << endl;
    cout <<  "timestamp = " << _bp.timestamp << endl;
    cout <<  "x = " << _bp.x << endl;
    cout <<  "y = " << _bp.y << endl;
}
//--------------------------------------------------------------
void ofApp::mainSoundCreate(int _i){
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
