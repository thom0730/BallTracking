#include "ofApp.h"

using namespace ofxCv;
using namespace cv;

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetWindowShape(800, 600);
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
    gui.add(Threshold.setup("Detect Threshold", -1, -5, 0));
    gui.add(DetectMAX.setup("MAX Attack", 200, 150, 400));
    gui.add(DetectMIN.setup("MIN Attack", 1.0, 0, 10.0));
    
    // OSC set up
    sender.setup(HOST, PORT);

    //軌跡が残るように
    ofSetBackgroundAuto(false);
    
    //カルマンフィルタの初期化
    for(int i = 0 ; i < BALL_NUM ; i ++ ){
        kalman[i].init(1/10000., 1/10.); // inverse of (smoothness, rapidness)
        line[i].setMode(OF_PRIMITIVE_LINE_STRIP);
        predicted[i].setMode(OF_PRIMITIVE_POINTS);
        estimated[i].setMode(OF_PRIMITIVE_POINTS);
    }

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
    /*
    //データを受信
    udpConnect.Receive((char*)&packet,(int)sizeof(packet));
    debug(packet);
  
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
    //======　カルマンフィルタ　======
    ofVec2f curPoint(packet.x, packet.y);
    ofVec2f Mpos(ofMap(curPoint.x,0,fullHD_x,0,ofGetWidth()), ofMap(curPoint.y,0,fullHD_y,0,ofGetHeight()));
    line[num].addVertex(Mpos);
    kalman[num].update(curPoint); // feed measurement カルマンフィルタに投げる
    // prediction before measurement 前の時刻の情報から推定した位置
    pre_pos[num] = kalman[num].getPrediction();
    ofVec2f Mpre_pos(ofMap(pre_pos[num].x,0,fullHD_x,0,ofGetWidth()), ofMap(pre_pos[num].y,0,fullHD_y,0,ofGetHeight()));
    predicted[num].addVertex(Mpre_pos);
    // corrected estimation after measurement 現在観測した情報に基づき新たに推定した位置
    est_pos[num] = kalman[num].getEstimation();
    cout <<"num = " << num <<endl;
    cout<< "now = " << curPoint << endl;
    cout << "future = " << est_pos[num] <<endl;
    ofVec2f Mest_pos(ofMap(est_pos[num].x,0,fullHD_x,0,ofGetWidth()), ofMap(est_pos[num].y,0,fullHD_y,0,ofGetHeight()));
    estimated[num].addVertex(Mest_pos);
    //移動速度
    speed[num] = kalman[num].getVelocity().length();
    int alpha = ofMap(speed[num], 0, 20, 50, 255, true);
    line[num].addColor(ofColor(255, 255, 255, 10));
    predicted[num].addColor(ofFloatColor(0.5, 0.8, 1, alpha)); //青線：前の時刻の情報から推定した位置
    estimated[num].addColor(ofColor(255, 155, 255, alpha)); //ピンク線：現在観測した情報に基づき新たに推定した位置
    //======　カルマンフィルタ　======
    bp[num] = packet; //配列に受信した構造体を格納
    buffering(bp[num],num); //座標位置をサンプリング用のバッファに格納
    attack[num] = 0; // 衝突判定の前に毎度Attackを初期化
    detect(bp[num],num);
    //detect2(num);
    mainSoundCreate(bp[num],num);
    sendOSC(bp[num], num);
    */
    

    
     //遅延する場合
    int num = 0;
    //attackの初期化
    for(int i = 0 ; i < BALL_NUM ; i++){
        attack[i] = 0;
    }
     while(1){
         udpConnect.Receive((char*)&packet,(int)sizeof(packet));
         //TIMELINE

     if(packet.ballId == 0){
         break;
     }
         debug(packet);
      //   ballpacket.push_back(packet);
         //IDを整理
         if(packet.x < fullHD_x/2){
             num = LEFT-1;
         }else{
             num = RIGHT-1;
         }
         //(0,0)を受信した時のIDの振り分け(左右が順番に送られてくる前提)
         if(packet.x == 0){
             //始めの(0,0)のときは、OSCで「0」を送信
             introFLG();
             switch(buffArrID){
                 case 0:
                     num = 1;
                     break;
                 case 1:
                     num = 0;
                     break;
             }
         }
         buffArrID = num;//1パケット前に受けたボールのID
         
          //======　カルマンフィルタ　======
         ofVec2f curPoint(packet.x, packet.y);
         ofVec2f Mpos(ofMap(curPoint.x,0,fullHD_x,0,ofGetWidth()), ofMap(curPoint.y,0,fullHD_y,0,ofGetHeight()));
         line[num].addVertex(Mpos);
         kalman[num].update(curPoint); // feed measurement カルマンフィルタに投げる
         // prediction before measurement 前の時刻の情報から推定した位置
         pre_pos[num] = kalman[num].getPrediction();
         ofVec2f Mpre_pos(ofMap(pre_pos[num].x,0,fullHD_x,0,ofGetWidth()), ofMap(pre_pos[num].y,0,fullHD_y,0,ofGetHeight()));
         predicted[num].addVertex(Mpre_pos);
         // corrected estimation after measurement 現在観測した情報に基づき新たに推定した位置
         est_pos[num] = kalman[num].getEstimation();
      //   cout <<"num = " << num <<endl;
      //   cout<< "now = " << curPoint << endl;
       //  cout << "future = " << est_pos[num] <<endl;
         ofVec2f Mest_pos(ofMap(est_pos[num].x,0,fullHD_x,0,ofGetWidth()), ofMap(est_pos[num].y,0,fullHD_y,0,ofGetHeight()));
         estimated[num].addVertex(Mest_pos);
         //移動速度
         speed[num] = kalman[num].getVelocity().length();
         int alpha = ofMap(speed[num], 0, 20, 50, 255, true);
         line[num].addColor(ofColor(255, 255, 255, 10));
         predicted[num].addColor(ofFloatColor(0.5, 0.8, 1, alpha)); //赤線：前の時刻の情報から推定した位置
         estimated[num].addColor(ofColor(255, 155, 255, alpha)); //緑線：現在観測した情報に基づき新たに推定した位置
        //======　カルマンフィルタ　======

         bp[num] = packet;
         buffering(bp[num] ,num); //左右それぞれの座標位置をバッファに格納
         detect(bp[num] ,num);
         mainSoundCreate(bp[num],num);
     }
    //OSC送信
    for(int i = 0 ; i < BALL_NUM ; i++){
        sendOSC(bp[i],i);
    }
    
     //初期化
    // ballpacket.clear();

}

//--------------------------------------------------------------
void ofApp::draw(){
    //X-Y展開
    ofSetColor(0, 0, 0, 20);
    ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());
    trackingDraw();
    
    //Y-T展開
    //graphDraw();
    
    //カルマンフィルタの結果を描画
    for(int i = 0 ; i < BALL_NUM ; i++){
        line[i].draw();
        predicted[i].draw(); //メッシュ描画
        ofPushStyle();
        ofSetColor(0, 0, 255, 5 );
        ofFill();
        ofVec2f Mpre_pos(ofMap(pre_pos[i].x,0,fullHD_x,0,ofGetWidth()), ofMap(pre_pos[i].y,0,fullHD_y,0,ofGetHeight()));
       // ofDrawCircle(Mpre_pos, speed[i] * 2);
        ofPopStyle();
        
        estimated[i].draw();

    }
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
    if(_i == (LEFT-1)){
        //SAMPLE_RATEフレーム前の座標位置から現在位置への速度ベクトル
        L_vec.set(_bp.x - Lprev_x[SAMPLE_RATE-1],_bp.y - Lprev_y[SAMPLE_RATE-1]);
       
        float x = L_vec.x * Lprev_vec.x;
        float y = L_vec.y * Lprev_vec.y;
         //前フレームの速度ベクトルと現在の速度ベクトルの「積が負」になれば速度ベクトルが逆転していると判定
        if(y < Threshold && L_vec.y < 0){
             cout <<"L_vec.y = " << L_vec.y << " | y = " << y << endl;
            //現在ベクトルのy方向大きさをattackとして出力
            attack[_i] = ABS(L_vec.y);
            
            //ボールが消えた時のattackの検出を外す
            if(attack[_i] > DetectMAX){
                attack[_i]  = 0;
            }
            //アタックが小さい場合のアンプ
            else if(attack[_i] > DetectMIN && attack[_i] < 10.0){
                 attack[_i] = 10.0;
            }
            //アタックが小さすぎる場合は除去
            else if(attack[_i] < DetectMIN){
                attack[_i] = 0.0;
            }
        }
          Lprev_vec = L_vec; //現在の速度ベクトルを格納
    }else{
        R_vec.set(_bp.x - Rprev_x[SAMPLE_RATE-1],_bp.y - Rprev_y[SAMPLE_RATE-1]);
        float x = R_vec.x * Rprev_vec.x;
        float y = R_vec.y * Rprev_vec.y;


        if(y < Threshold && R_vec.y < 0){
            cout <<"R_vec.y = " << R_vec.y << " | y = " << y << endl;
            attack[_i] = ABS(R_vec.y);
            if(attack[_i] > DetectMAX ){
                attack[_i]  = 0;
            }
            else if(attack[_i] > DetectMIN && attack[_i] < 10.0){
                attack[_i] = 10.0;
            }
            else if(attack[_i] < DetectMIN){
                attack[_i] = 0.0;
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
                if(attack[_i] != 0){
                    cout << "OSC = " << attack[_i] << endl;
                }
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
void ofApp::introFLG(){
    
    if (!flg){
        ofxOscMessage m;
        string msg = "";
        msg += "/Introduction";
        m.setAddress(msg);
        m.addIntArg(0);
        sender.sendMessage(m);
        cout <<" ____SEND 0____ "<< endl;
        flg = true;
    }
    
}
//--------------------------------------------------------------
void ofApp::keyPressed(int key){

    if(key == ' ' ){
        introFLG();
    }
    
    //MARKER FOR DEBUG
    if(key == 'v'){
        cout << "GOOD" << endl;
    }
    if(key == 'b'){
        cout << "BAD" << endl;
    }
    
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
void ofApp::exit(){
    if(isBind) udpConnect.Close();
}
