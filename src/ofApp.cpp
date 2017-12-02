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
    
    /*============================
     画面左：bp[0] BALL1 (LEFT = 1)
     画面右：bp[1] BALL2 (RIGHT = 2)
     ============================*/
    //データを受信
    udpConnect.Receive((char*)&packet,(int)sizeof(packet));
    //debug(packet);
    //IDを整理
    int num = 0;
    if(packet.x < fullHD_x/2){
        num = LEFT-1;
    }else{
        num = RIGHT-1;
    }
    //(0,0)を受信した時のIDの振り分け(左右が順番に送られてくる前提)
    if(packet.ballId == 0){
        switch(buffArrID){
            case 0:
                num = 1;
                break;
            case 1:
                num = 0;
                break;
        }
    }
    buffArrID = num; //前フレームに受けたボールのID

    bp[num] = packet; //配列に受信した構造体を格納
    buffering(bp[num],num); //座標位置をサンプリング用のバッファに格納
    detect(bp[num],num);
    mainSoundCreate(bp[num],num);
    sendOSC(bp[num], num);

    /*
     //遅延する場合
     while(1){
     udpConnect.Receive((char*)&packet,(int)sizeof(packet));
     debug(packet);
     if(packet.ballId == 0){
     break;
     }
     ballpacket.push_back(packet);
     buffering(packet);
     }
     
     for(int i = 0 ; i < BALL_NUM ; i ++){
     detect2(i);
     mainSoundCreate(i);
     sendOSC(ballpacket[i], i);
     }
     //初期化
     ballpacket.clear();
     */
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofSetColor(0, 0, 0, 5);
    
    //X-Y展開
    ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());
    trackingDraw();
    
    //Y-T展開
    //graphDraw();

    //方眼紙
    drawdata.drawGrid();
    // GUIを表示
    gui.draw();
    

}
//--------------------------------------------------------------
void ofApp::buffering(BallPacket _bp, int _i){
    if(_i == (LEFT-1)){
        for(int i = SAMPLE_RATE-1 ; i > 0 ; i --){
            Lprev_x[i] = Lprev_x[i-1];
            Lprev_y[i] = Lprev_y[i-1];
            Lprev_x[0] = _bp.x;
            Lprev_y[0] = _bp.y;
        }
    }else{
        for(int i = SAMPLE_RATE-1 ; i > 0 ; i --){
            Rprev_x[i] = Rprev_x[i-1];
            Rprev_y[i] = Rprev_y[i-1];
        }
        Rprev_x[0] = _bp.x;
        Rprev_y[0] = _bp.y;
    }

}
//--------------------------------------------------------------
void ofApp::detect(BallPacket _bp, int _i){
    attack[_i] = 0;
    if(_i == (LEFT-1)){
        //SAMPLE_RATEフレーム前の座標位置から現在位置への速度ベクトル
        L_vec.set(_bp.x - Lprev_x[SAMPLE_RATE-1],_bp.y - Lprev_y[SAMPLE_RATE-1]);
       
        float x = L_vec.x * Lprev_vec.x;
        float y = L_vec.y * Lprev_vec.y;
         //前フレームの速度ベクトルと現在の速度ベクトルの「積が負」になれば速度ベクトルが逆転していると判定
        if(y < Threshold && L_vec.y > 0){
            //現在ベクトルのy方向大きさをattackとして出力
            attack[_i] = ABS(L_vec.y);
            
            //ボールが消えた時のattackの検出を外す
            if(attack[_i] > 200){
                attack[_i]  = 0;
            }
            //アタックが小さい場合のアンプ
            else if(attack[_i] < 10.0){
                 attack[_i] = 10.0;
            }
        }
          Lprev_vec = L_vec; //現在の速度ベクトルを格納
    }else{
        R_vec.set(_bp.x - Rprev_x[SAMPLE_RATE-1],_bp.y - Rprev_y[SAMPLE_RATE-1]);
        float x = R_vec.x * Rprev_vec.x;
        float y = R_vec.y * Rprev_vec.y;


        if(y < Threshold && R_vec.y < 0){
             cout <<"R_vec.y = " << R_vec.y << " y = " << y << endl;
            attack[_i] = ABS(R_vec.y);
            if(attack[_i] > 200 ){
                attack[_i]  = 0;
            }
            else if(attack[_i] < 10.0){
                 attack[_i] = 10.0;
            }
 
        }
        Rprev_vec = R_vec;
    }

}
//--------------------------------------------------------------
void ofApp::mainSoundCreate(BallPacket _bp,int _i){
    //音色分け
    if(_bp.y < ofGetHeight()/3){
        note[_i] = 1 * (_i+1);
    }else if(_bp.y > ofGetHeight()/3 && _bp.y < 2*ofGetHeight()/3){
        note[_i] = 2 * (_i+1);
    }else{
        note[_i] = 3 * (_i+1);
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
void ofApp::detect1(int _i){
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

        //2フレーム前のバッファ
        bbuffx[_i] = buffx[_i];
        bbuffx[_i] = buffx[_i];
        bbuffvec[_i] =  buffvec[_i];
        //現在座標をバッファに格納し、次フレームでのベクトル生成に使用
        buffx[_i] = bp[_i].x;
        buffy[_i] = bp[_i].y;
        buffvec[_i] = vec[_i];
    }
    
}
//--------------------------------------------------------------
void ofApp::detect2(int _i){
    attack[_i] = 0;

    //前のフレームの座標->現在座標の速度ベクトルを生成
    vec[_i].set(bp[_i].x - buffx[_i] , bp[_i].y - buffy[_i]);
    //前フレームで生成したベクトルと現在ベクトルの積が負になれば速度ベクトルが逆転していると判定
    float x = vec[_i].x * buffvec[_i].x;
    float y = vec[_i].y * buffvec[_i].y;
    
    if(y < Threshold && vec[_i].y < 0){
        //現在ベクトルの大きさをattackとして出力
        attack[_i] = ABS(bp[_i].y - buffy[_i]);

        //ボールが消えた時のattackの検出を外す
        if(attack[_i] > 200 ){
            attack[_i]  = 0;
        }
        else if(attack[_i] > 2.5 && attack[_i] < 10.0){
           // attack[_i] = 10.0;
        }else if(attack[_i] < 2.1 ){
            //attack[_i] = 0.0;
        }
    }
    //現在座標をバッファに格納し、次フレームでのベクトル生成に使用
    buffx[_i] = bp[_i].x;
    buffy[_i] = bp[_i].y;
    buffvec[_i] = vec[_i];

}
//--------------------------------------------------------------
void ofApp::detect3(int _i){
    //2フレーム前のバッファ
    bbuffx[_i] = buffx[_i];
    bbuffy[_i] = buffy[_i];
    
    //ローパスフィルタ
    lowpassFilter(buffx[_i],bp[_i].x);
    lowpassFilter(buffy[_i],bp[_i].y);
    //前のフレームの座標->現在座標の速度ベクトルを生成
    vec[_i].set(bp[_i].x - bbuffx[_i] , bp[_i].y - bbuffy[_i]);
    //前フレームで生成したベクトルと現在ベクトルの積が負になれば速度ベクトルが逆転していると判定
    float x = vec[_i].x * buffvec[_i].x;
    float y = vec[_i].y * buffvec[_i].y;

    attack[_i] = 0;
    if(y < Threshold && vec[_i].y < 0){
        //現在ベクトルの大きさをattackとして出力
        attack[_i] = ABS(bp[_i].y - bbuffy[_i]);
        
        //ボールが消えた時のattackの検出を外す
     /*   if(attack[_i] > 400 ){
            attack[_i]  = 0;
        }else if(attack[_i] > 1.0 && attack[_i] < 10.0){
            attack[_i] = 10.0;
        }*/
    }

    //現在座標をバッファに格納し、次フレームでのベクトル生成に使用
    buffx[_i] = bp[_i].x;
    buffy[_i] = bp[_i].y;
    buffvec[_i] = vec[_i];
}
//--------------------------------------------------------------
void ofApp::trackingDraw(){

    ofSetColor(0, 0, 0);
    ofDrawRectangle(0, 0, ofGetWidth(), 110);
    for(int i = 0 ; i < BALL_NUM ; i++){
        ofSetColor(255, 255, 255);
        //動画サイズ(フルHD)をWindow Sizeに変換
        float x = ofMap(bp[i].x,0,fullHD_x,0,ofGetWidth());
        float y = ofMap(bp[i].y,0,fullHD_y,0,ofGetHeight());
        
        ofDrawBitmapString( "Ball ID = " + ofToString(i+1) , (i+1)*300, 20);
        ofDrawBitmapString( "x = " + ofToString(x) , (i+1)*300, 40);
        ofDrawBitmapString( "y = " + ofToString(y) , (i+1)*300, 60);
        ofDrawBitmapString( "attack = " + ofToString(attack[i]) , (i+1)*300, 80);
        ofDrawBitmapString( "note = " + ofToString(note[i]) , (i+1)*300, 100);

        //ball
        int j = 0;
        if(attack[i] != 0){
            j = 10;
        }
         ofSetColor(i*100 + j, 100*(i+1), 155 + j);
      //  ofDrawBitmapString( "ID = " + ofToString(i), x+5,y+5);
        ofDrawCircle(x,y, 3 + j);
        
        //目盛
        for(int i = 0 ; i < ofGetHeight() ; i ++){
            if((20*i)%100 == 0){
                ofSetColor(255, 0, 0 );
                ofDrawBitmapString( ofToString(i*20) , 10, 20*i);
            }
        }
    }
    //FPSの表示
    ofSetColor(255, 255, 255);
    ofDrawBitmapString(ofToString(ofGetFrameRate())+ "fps" , ofGetWidth() - 100, 20);
}
//--------------------------------------------------------------
void ofApp::graphDraw(){
    
    for(int i = 0 ; i <2 ; i++){
        ofSetColor(255, 255, 255);
        //動画サイズ(フルHD)をWindow Sizeに変換
        float x = ofMap(bp[i].x,0,fullHD_x,0,ofGetWidth());
        float y = ofMap(bp[i].y,0,fullHD_y,0,ofGetHeight());
        
        int j = 0;
        if(attack[i] != 0){
            j = 100;
            ofDrawBitmapString( "ID = " + ofToString(i), 10+ofMap(countFrame,0,2000,0,ofGetWidth()),y+40);
            ofDrawBitmapString( "Attack = " + ofToString(attack[i]), 10+ofMap(countFrame,0,2000,0,ofGetWidth()),y+60);
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
    
    //FPSの表示
    ofSetColor(255, 255, 255);
    ofDrawBitmapString(ofToString(ofGetFrameRate())+ "fps" , ofGetWidth() - 100, 20);

}
//--------------------------------------------------------------
void ofApp::lowpassFilter(float _posPrev, float _posNew){
    _posPrev = alpha * _posPrev + (1-alpha) * _posNew;

}
//--------------------------------------------------------------
void ofApp::debug(BallPacket _bp){
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
