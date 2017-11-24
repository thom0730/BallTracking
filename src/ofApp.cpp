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
  // debug(packet);

    /*============================
     画面右：bp[0] BALL1 (RIGHT = 1)
     画面左：bp[1] BALL2 (LEFT = 2)
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
    //衝突検出
    detect(number);
    
    //音の処理のタイムライン
    startIntro();
    if(introFlg){
        startCount();
    }
    if(mainFlg){
        for(int i = 0  ; i < BALL_NUM ; i++){
             mainSoundCreate(i);
        }
    }
    //OSC 送信
    for(int i = 0  ; i < BALL_NUM ; i++){
        sendOSC(bp[i], i);
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    // 画面をフェード
    ofSetColor(0, 0, 0, 5);
    ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());
    
    ofSetColor(255, 255, 255);
    //文字表示画面の背景
    ofSetColor(0, 0, 0);
    ofDrawRectangle(0, 0, ofGetWidth(),100);
    
    for(int i = 0 ; i < BALL_NUM ; i++){
        ofSetColor(255, 255, 255);
        //動画サイズ(フルHD)をWindow Sizeに変換
        float x = ofMap(bp[i].x,0,fullHD_x,0,ofGetWidth());
        float y = ofMap(bp[i].y,0,fullHD_y,0,ofGetHeight());
        
        int j;
        if(i == 0){ //RIGHT
            j = 1;
            ofDrawBitmapString( "Ball ID = " + ofToString(RIGHT) , (j+1)*300, 20);
        }else{ //LEFT
            j = 0 ;
            ofDrawBitmapString( "Ball ID = " + ofToString(LEFT) , (j+1)*300, 20);
        }
        ofDrawBitmapString( "x = " + ofToString(x) , (j+1)*300, 40);
        ofDrawBitmapString( "y = " + ofToString(y) , (j+1)*300, 60);
        ofDrawBitmapString( "attack = " + ofToString(VecSize[i]) , (j+1)*300, 80);
        ofDrawBitmapString( "note = " + ofToString(note[i]) , (j+1)*300, 100);

        //ball
        ofSetColor(i*100 + ofMap(VecSize[i],0,1500,0,255), 255, 255);
        ofDrawBitmapString( "ID = " + ofToString(i), x+5,y+5);
        ofDrawCircle(x,y, 5);
        
        //速度ベクトル
        float buffX = ofMap(buffx[i],0,fullHD_x,0,ofGetWidth());
        float buffY = ofMap(buffy[i],0,fullHD_y,0,ofGetHeight());
        ofSetColor(255, 0, 0);
        ofDrawLine(x, y, buffX, buffY); //現在ベクトル
        float vecX = -1 * ofMap(vec[i].x,0,fullHD_x,0,ofGetWidth());
        float vecY = -1 * ofMap(vec[i].y,0,fullHD_y,0,ofGetHeight());
        ofSetColor(0, 255, 0);
        ofDrawLine(buffX, buffY, buffX+vecX, buffY+vecY); //バッファベクトル

    }

    ofDrawBitmapString(ofToString(ofGetFrameRate())+ "fps" , ofGetWidth() - 100, 20);
    if(introFlg){
        ofDrawBitmapString("Introduction" , ofGetWidth() - 300, 20);
        if(!introCue){
            ofDrawBitmapString("measure："+ofToString(measure_num), ofGetWidth() - 300, 40);
        }
    }else if(mainFlg){
        ofDrawBitmapString("Main Performance" , ofGetWidth() - 300, 20);
    }
  
    //方眼紙
    drawGrid();
    // GUIを表示
    gui.draw();
    

}
//--------------------------------------------------------------
void ofApp::detect(int _i){
    VecSize[_i] = 0;
    
    if(countFrame%SAMPLE_RATE == 0){
        //前のフレームの座標->現在座標のベクトルを生成
        vec[_i].set(bp[_i].x - buffx[_i] , bp[_i].y - buffy[_i]);
        //前フレームで生成したベクトルと現在ベクトルの積が負になれば速度ベクトルが逆転していると判定
        float producXx = vec[_i].x * buffvec[_i].x;
        float productY = vec[_i].y * buffvec[_i].y;
        
        if(vec[_i].dot(buffvec[_i])<0 && buffvec[_i].y < 0){
      //  if(productY < Threshold && buffvec[_i].y < 0){
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
                m.addFloatArg(VecSize[_i]);
                break;
            case 3:
                if(introFlg || mainFlg){
                    msg += "/Ball" + ofToString(BallID) + "_note";
                }else{
                     msg += "/Ball" + ofToString(BallID) + "_intro_note";
                }
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
void ofApp::lowpassFilter(ofVec3f _vecPrev, ofVec3f _vecNew){
    _vecPrev = alpha * _vecPrev + (1-alpha) * _vecNew;

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
void ofApp::introSoundCreate(){
    introTime = (ofGetElapsedTimeMillis()-startTime)/100;
    measure_num = (introTime/int(BPM))+1; //小節数(1~10)
    switch (measure_num) {
        case 1:
            note[0] = 1;
            break;
        case 2:
            note[1] = 2;
            break;
        case 4:
            note[0] = 3;
            break;
        case 6:
            note[1] = 4;
            break;
        case 11:
            introFlg = false;
            mainFlg = true;
            break;
        default:
            break;
    }
}
//--------------------------------------------------------------
void ofApp::keyPressed(int key){ //本当はOSCで1を受けたタイミング
    if(key == 's'){
        startTime = ofGetElapsedTimeMillis();
        introCue = false;
    }
}
//--------------------------------------------------------------
void ofApp::startIntro(){
        if(countFrame > 600 && !mainFlg){
            if(bp[LEFT-1].x == 0.0){
                introFlg = true;
            }
        }
}
//--------------------------------------------------------------
void ofApp::startCount(){
    if(VecSize[0] != 0 && introCue){ //Note=1の最初のAttackを検出 & introCueがTrue(1周もしていない) => introduction：カウント開始
        startTime = ofGetElapsedTimeMillis();
        introCue = false;
    }
    if(!introCue){
        introSoundCreate();
    }
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
