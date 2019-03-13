from m5stack import *
from machine import UART
import time
import ambient

# 起動時の時刻を変数に入れる
lastTime = time.time()

# Ambientの初期設定
am = ambient.Ambient("AmbientのチャンネルIDに書き換えてください", "Ambientのリードキーに書き換えてください")

# LCDの前景色と背景色を黒に設定
lcd.setColor(lcd.BLACK, lcd.BLACK)

# 背景色で画面をクリアする
lcd.clear()

# LCDのフォントスタイルを設定
lcd.font(lcd.FONT_Default)

# UARTクラスはシリアル通信プロトコルを実装しています
# このクラスを使用すると、シリアルインタフェースを使用してM5Stackに接続されたデバイス（今回はMH-Z19）と通信できます
# txとrxにはそれぞれM5StackのGPIOピン番号を指定します
uart = UART(2, tx=17, rx=16)

# MH-Z19Bのデータシートに記載された設定を参考にして、通信方法を設定します
uart.init(9600, bits=8, parity=None, stop=1)

def getPPM():
    global uart
    
    # ガス濃度を読み込むのに使うコマンドをバイト配列で用意して、書き込みます
    b = bytearray(b'\xff\x01\x86\x00\x00\x00\x00\x00\x79')
    uart.write(b)
    
    # cに9バイトまでバイト配列を読み込みます
    c = bytearray(9)
    uart.readinto(c, 9)
    
    # 返ってきたバイト配列から二酸化炭素濃度の値を取り出します
    ppm = c[2]*256+c[3]
    
    return ppm

def getVentilationStatus(ppm):
    
    if ppm <= 410:
        status = "Waiting..."
    elif ppm <= 450:
        # 350～450ppm 過剰な換気(外気:330～400ppm)
        status = "Excess"
    elif ppm <= 700:
        # 450～700ppm 理想的な換気レベル
        status = "Good"
    elif ppm <= 1000:
        # 700～1000ppm 換気が不十分（室内では1000ppm以下に抑えることとされている）
        status = "Notice"
    elif ppm <= 1500:
         # 1000～1500ppm 悪い室内空気環境（学校環境では1500ppm以下が望ましいとされいる）
        status = "Caution"
    elif ppm <= 5000:
        # 1500～5000ppm これ以上の環境で労働をしてはいけない（労働安全基準法では、5000ppmが限度）
        status = "Warning"
    elif ppm <= 6000:
        # 5000ppm以上 疲労集中力の欠如
        status = "Danger"
    else:
        status = "Waiting..."

    return status


def savePPMToAmbient(ppm):
    global lastTime

    # 現在時刻を取得
    currentTime = time.time()

    # Ambientにデータ送ってから60秒以上経過していたら、再度データ送信を行う
    if 60 < (currentTime - lastTime):
        # MH-Z19Bは起動直後は正常な値を返さないので、正常な値が返ってきてない場合はログに残さない
        if (410 <= ppm) and (ppm <= 6000):
            
            # Ambient初期設定時に設定したチャンネルのd1に二酸化炭素濃度の数値を送信
            r = am.send({'d1': ppm})
            print(r.status_code)
            r.close()

            # Ambientにデータを送った最終時間を記録
            lastTime = currentTime

def displayPPM(ppm):
    # 二酸化炭素濃度をLCDに表示する
    lcd.font(lcd.FONT_DejaVu24)
    lcd.text(10,100, str(ppm) + " ppm", lcd.WHITE)

def displayVentilationStatus(ventilationStatus):
    lcd.font(lcd.FONT_DejaVu24)
    lcd.text(10,200, ventilationStatus, lcd.WHITE)

while True:
    # 二酸化炭素濃度を取得する
    ppm = getPPM()
    
    # 二酸化炭素濃度をLCDに表示
    displayPPM(ppm)
    
    # 換気ステータスを取得
    ventilationStatus = getVentilationStatus(ppm)
    
    # 換気ステータスをLCDに表示
    displayVentilationStatus(ventilationStatus)
    
    # Ambientにppmの数値を保存する
    savePPMToAmbient(ppm)
    
    # 指定の秒数 sleepする。ここでは10秒sleepさせてる
    time.sleep(10)
    
    # 背景色で画面をクリアする
    lcd.clear()