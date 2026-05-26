#include "widget.h"
#include "ui_widget.h"
#include "fftstuff.h"
#include "fileloader.h"
#include <qtimer.h>
#include <math.h>
#include <stdlib.h>
#include <iostream>
#include <QFile>
#include <QPainter>
#include <QThread>
#include <QMessageBox>

int test_val = 0;
int curLessonInt = 0;
int orientation [21] = {1,2,3,4,5,6,7,8,1,8,1,8,1,8,7,6,5,4,3,2,1};
QMap<QString, int> tonic_map = {
    {"G3", 43}, {"A3", 45}, {"B3", 47}, {"C4", 48}, {"D4", 50}, {"A4", 52}, {"B4", 54}, {"C5", 55}
};
QMap<int, QString> tilesGoodBadMap = {
    {0, "note0"}, {1, "note2"}, {2, "note4"}, {3, "note5"}, {4, "note7"}, {5, "note9"},
    {6, "note11"}, {7, "note12"}
};
QMap<int, int> tileKbShift = {
    {0, 0}, {1, 2}, {2, 4}, {3, 5}, {4, 7}, {5, 9}, {6, 11}, {7, 12}
};
FftStuff fts;
QThread speakerThread;
QThread microphoneThread;
extern bool collectMicData;
extern double rec_arr[];    // DEFINED AS DOUBLE FOR FFTW
extern int rec_arr_cnt;
extern int frame_start;
extern int frame_size;
extern int frame_end;
extern int rec_arr_end;
QByteArray bufferReadTo;
QByteArray *currentAudioOut;
// QList<QByteArray> note_tone;
int major_number_list[] = {0,2,4,5,7,9,11,12};
int tonic_nunber;
int audio_number_list;
const QList <QString> note_letters = {"C", "C#", "D", "D#", "E",
                                     "F", "F#", "G", "G#", "A", "A#", "B" };
extern float note_acc;
extern int noteC_no;
extern int noteC_oct;
extern float nn_l[12];
extern QList<QByteArray> rawRecArrays;
int accValue = 0;
int displayDuration = 3000;


Microphone::Microphone(const QAudioFormat &format) : m_format(format) {
    qDebug()<<" YOU SHOULD SEE THIS ";
}

void Microphone::start()
{
    collectMicData=true;
    open(QIODevice::WriteOnly);
}

void Microphone::stop()
{
    close();
    frame_start = 0;
}

qint64 Microphone::readData(char * /* data */, qint64 /* maxlen */)      // NOT USED IN EXAMPLE
{
    return 0;
}

qreal Microphone::getNoteValue(const char *data, qint64 len) const
{
    const int channelBytes = m_format.bytesPerSample();
    const int sampleBytes = m_format.bytesPerFrame();
    const int numSamples = len / sampleBytes;

    float maxValue = 0;
    auto *ptr = reinterpret_cast<const unsigned char *>(data);

    for (int i = 0; i < numSamples; ++i) {
        float value = m_format.normalizedSampleValue(ptr);
        rec_arr[rec_arr_cnt]=value;     //  --------> DEFINED AS DOUBLE FOR FFTW  but value is a float
        //maxValue = qMax(value, maxValue);
        ptr += channelBytes;
        rec_arr_cnt++;
    };
    // qDebug()<<"    END OF DATA FROM MIC---------->   rec_arr_cnt "<<rec_arr_cnt;

    if(rec_arr_cnt > frame_end){
        cout<<"\n mic NEXT FRAME $$$$ "<<frame_start<<endl;

        fts.DoIt(frame_start, frame_size);
        frame_start = frame_end;
        frame_end = frame_end + frame_size;
    }

    if (rec_arr_cnt > 200000)
    {
        qDebug()<<"  200000 hit value " << collectMicData;
        rec_arr_cnt = 0;
        frame_start = 0;
        frame_end = frame_end + frame_size;
        qDebug() << ">>>>>>>>zero position at 200000 " << rec_arr_cnt;

        emit on_timeOut();

        qDebug() <<"                      restart here";
        qDebug() <<"                   Microphone::pos()  "<<Microphone::pos();
    }
    test_val++;
    qDebug() <<"Microphone::pos()  "<<Microphone::pos();
    qDebug() << ">>> " << test_val;
    return maxValue;
}

qint64 Microphone::writeData(const char *data, qint64 len)
{
    if(collectMicData)
    {
        //qDebug() <<" %%%%%% :writeData(const char *data, qint64 len)  "<<&data<<" len "<<len;
        m_level = getNoteValue(data, len);
    }
    return len;
}

Speaker::Speaker()
{

}

void Speaker::start()
{
    open(QIODevice::ReadOnly);
}

void Speaker::stop()
{
    m_pos = 0;
    close();
}

void Speaker::newTest(QByteArray bufferOut)
{
    qDebug()<<"  %%%%  NEW TEST ";
    m_buffer.assign(bufferOut);
    m_buffer.clear();
    qDebug()<<&m_buffer<<" m_buffer_.size() " << m_buffer.size();
    m_buffer = bufferOut;
    qDebug()<<&m_buffer<<" m_buffer_.size() " << m_buffer.size();
}

qint64 Speaker::readData(char *data, qint64 len)
{
    qDebug() << "enter Speaker readdata...is Main? " << QThread::isMainThread();
    qint64 total = 0;
    if (!m_buffer.isEmpty()) {
        while (len - total > 0) {
            const qint64 chunk = qMin((m_buffer.size() - m_pos), len - total);
            memcpy(data + total, m_buffer.constData() + m_pos, chunk);
            m_pos = (m_pos + chunk) % m_buffer.size();
            total += chunk;
            qDebug() << "chunk..." << chunk << "pos> = " << m_pos ;
        }
    }
    qDebug() << ">>is Main Thread in speaker readData: " << QThread::isMainThread();
    return total;
}

qint64 Speaker::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

qint64 Speaker::bytesAvailable() const
{
    return QIODevice::bytesAvailable();
}

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->setWindowTitle("Ear Trainer Qt6 version 6.10.2");
    this->setWindowIcon(QIcon(":/img/note.png"));
    QPixmap pix(":/img/down-arrow.png");
    ui->lb_arrow->setPixmap(pix);
    FileLoader::ReadConfig();
    FileLoader::ReadLesson();
    FileLoader::GetRandomTestSet(gTestGroup[curLessonInt]);
    initializeAudioOutput(m_devicesOut->defaultAudioOutput());
    initializeAudioInput(QMediaDevices::defaultAudioInput());
    ui->btnNext->setVisible(false);
    ui->lb_review->setText("Start");
    ui->lb_title->setText("Lesson");
    accValue = 0;
    ui->lb_arrow->move(800, 100);

    connect(ui->btnStart, &QPushButton::clicked, this,&Widget::startSound);
    connect(ui->btnStart, &QPushButton::clicked, this,&Widget::restartAudioStream);
    connect(&fts, &FftStuff::on_foundNote, this, &Widget::micFoundNote);
    connect(&fts, &FftStuff::valueChanged,this, &Widget::updateKBnote);
    connect(m_Microphone.data(), &Microphone::on_timeOut, this, &Widget::timeOut);
}

Widget::~Widget()
{
    SpeakerThread.quit();
    MicThread.quit();
    delete ui;
}

void Widget::paintEvent(QPaintEvent *event)
{
    QPixmap pix(100,60);
    pix.fill(Qt::white);
    //create a QPainter and pass a pointer to the device.
    //A paint device can be a QWidget, a QPixmap or a QImage
    QPainter *painter = new QPainter(&pix);
    QPen outsidePen(Qt::red, 4, Qt::SolidLine);
    painter->setPen(outsidePen);
    painter->drawEllipse(35, 15, 30, 30);
    QPen insidePen(Qt::green, 4, Qt::SolidLine);
    painter->setPen(insidePen);
    painter->drawEllipse(40 + accValue, 20, 20, 20);
    QPen linePen(Qt::black, 1, Qt::SolidLine);
    painter->setPen(linePen);
    painter->drawLine(0, 30, 100, 30);
    painter->drawLine(50, 0, 50, 60);
    painter->end();
    ui->lb_tuner->setPixmap(pix);
    orientationFlag = true;
}

void Widget::updateKBnote(int kbValue, float acc)
{
    int letter = kbValue%12;
    int octaveValue = kbValue/12;
    QString theNote = note_letters[letter] + QString::number(octaveValue);
    qDebug() << "knNote... " << theNote;
    QString alphaNote = note_letters[letter] + QString::number(octaveValue);
    qDebug() << "alphaNote... " << alphaNote;
    ui->lb_note->setText(alphaNote);
    qDebug() << "acc is: " << (int)(acc * 1000);
    accValue = (int)(acc * 1000);
    ui->lb_tuner->repaint();
}

void Widget::timeOut()
{
    m_Microphone->seek(0);
    MicThread.exit();
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Timed Out", "Continue?",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        qDebug() << "continuing...";
        rec_arr_cnt = 0;
        m_Speaker->start();
        MicThread.start();
        m_audioSource->stop();
        m_audioSource->setVolume(1.0);
        m_audioSource->start(m_Microphone.get());
        // m_Microphone->start();
        // m_audioSource.reset();
        // m_audioSource->start();
    } else {
        qDebug() << "No was clicked";
        QApplication::quit();
    }    
}

void Widget::micFoundNote(int value)
{
    MicThread.exit();
    m_Microphone->seek(0);
    m_Microphone->reset();
    m_Microphone->stop();
    qDebug() << ">>>>MicThread is running>>>>> " << MicThread.isRunning();

    qDebug() << "-->note found: " << value;
    stopSound();
    heardNote = value;
    ui->lb_arrow->move(10+((heardNote - tonicNote)*45), 100);
    QThread::msleep(1000);
    kbPlayedNote = tonicNote + tileKbShift[playedNote];
    qDebug() << "-->heardNote: " << heardNote << " = " << kbPlayedNote;
    switch (playedNote){
    case 0:
        if (kbPlayedNote == heardNote)
        {
            ui->note0->setStyleSheet("QLabel { background-color: green;"
                                     "border-style: outset;"
                                     " border-width: 3px; border-color: black;}");
        } else {
            ui->note0->setStyleSheet("QLabel { background-color: red;"
                                     "border-style: outset;"
                                     " border-width: 3px; border-color: black;}");
        }
        break;
    case 1:
        if (kbPlayedNote == heardNote)
        {
            ui->note1->setStyleSheet("QLabel { background-color: green;"
                                     "border-style: outset;"
                                     " border-width: 3px; border-color: black;}");
        } else {
            ui->note1->setStyleSheet("QLabel { background-color: red;"
                                     "border-style: outset;"
                                     " border-width: 3px; border-color: black;}");
        }
        break;
    case 2:
        if (kbPlayedNote == heardNote)
        {
            ui->note2->setStyleSheet("QLabel { background-color: green;"
                                     "border-style: outset;"
                                     " border-width: 3px; border-color: black;}");
        } else {
            ui->note2->setStyleSheet("QLabel { background-color: red;"
                                     "border-style: outset;"
                                     " border-width: 3px; border-color: black;}");
        }
        break;
    case 3:
        if (kbPlayedNote == heardNote)
        {
            ui->note3->setStyleSheet("QLabel { background-color: green;"
                                     "border-style: outset;"
                                     " border-width: 3px; border-color: black;}");
        } else {
            ui->note3->setStyleSheet("QLabel { background-color: red;"
                                     "border-style: outset;"
                                     " border-width: 3px; border-color: black;}");
        }
        break;
    case 4:
        if (kbPlayedNote == heardNote)
        {
            ui->note4->setStyleSheet("QLabel { background-color: green;"
                                     "border-style: outset;"
                                     " border-width: 3px; border-color: black;}");
        } else {
            ui->note4->setStyleSheet("QLabel { background-color: red;"
                                     "border-style: outset;"
                                     " border-width: 3px; border-color: black;}");
        }
        break;
    case 5:
        if (kbPlayedNote == heardNote)
        {
            ui->note5->setStyleSheet("QLabel { background-color: green;"
                                     "border-style: outset;"
                                     " border-width: 3px; border-color: black;}");
        } else {
            ui->note5->setStyleSheet("QLabel { background-color: red;"
                                     "border-style: outset;"
                                     " border-width: 3px; border-color: black;}");
        }
        break;
    case 6:
        if (kbPlayedNote == heardNote)
        {
            ui->note6->setStyleSheet("QLabel { background-color: green;"
                                     "border-style: outset;"
                                     " border-width: 3px; border-color: black;}");
        } else {
            ui->note6->setStyleSheet("QLabel { background-color: red;"
                                     "border-style: outset;"
                                     " border-width: 3px; border-color: black;}");
        }
        break;
    case 7:
        if (kbPlayedNote == heardNote)
        {
            ui->note7->setStyleSheet("QLabel { background-color: green;"
                                     "border-style: outset;"
                                     " border-width: 3px; border-color: black;}");
        } else {
            ui->note7->setStyleSheet("QLabel { background-color: red;"
                                     "border-style: outset;"
                                     " border-width: 3px; border-color: black;}");
        }
    }
    ui->note0->repaint();
    QThread::currentThread()->msleep(displayDuration);
    ui->note0->setStyleSheet("QLabel { background-color: white;"
                             "border-style: outset;"
                             " border-width: 3px; border-color: black;}");
    ui->note1->setStyleSheet("QLabel { background-color: white;"
                             "border-style: outset;"
                             " border-width: 3px; border-color: black;}");
    ui->note2->setStyleSheet("QLabel { background-color: white;"
                             "border-style: outset;"
                             " border-width: 3px; border-color: black;}");
    ui->note3->setStyleSheet("QLabel { background-color: white;"
                             "border-style: outset;"
                             " border-width: 3px; border-color: black;}");
    ui->note4->setStyleSheet("QLabel { background-color: white;"
                             "border-style: outset;"
                             " border-width: 3px; border-color: black;}");
    ui->note5->setStyleSheet("QLabel { background-color: white;"
                             "border-style: outset;"
                             " border-width: 3px; border-color: black;}");
    ui->note6->setStyleSheet("QLabel { background-color: white;"
                             "border-style: outset;"
                             " border-width: 3px; border-color: black;}");
    ui->note7->setStyleSheet("QLabel { background-color: white;"
                             "border-style: outset;"
                             " border-width: 3px; border-color: black;}");

    ui->lb_arrow->move(800, 100);
    collectMicData = true;
    m_Microphone->seek(0);
    m_Microphone->stop();
    m_Microphone->start();
    m_audioSource->stop();
    qDebug()<<" restartAudioStream() |  m_pullMode  "<<m_pullMode;
    m_audioSource->start(m_Microphone.get());
    test_val = 0;
    if (orientationFlag == true)
    {
        do_Orientation(nPos);
        m_Microphone->seek(0);
        if(nPos == 20)
        {
            orientationFlag=false;
            nPos = 0;
        }
    } else {
        do_Quiz(nPos);
        if(nPos == 19)
        {
            orientationFlag=true;
            qDebug() << "test complete";
        }
    }
    nPos++;
}

void Widget::initializeAudioOutput(const QAudioDevice &deviceInfo)
{
    qDebug() << "initializeAudioOutput...";
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
    qDebug() << "     !!!   from  INIT format: " << format.sampleRate();
    m_Speaker.reset(new Speaker());
    m_audioOutput.reset(new QAudioSink(deviceInfo, format));
}

void Widget::initializeAudioInput(const QAudioDevice &deviceInfo)
{
    qDebug() << "initializeAudioInput...";
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    m_Microphone.reset(new Microphone(format));
    m_audioSource.reset(new QAudioSource(deviceInfo, format));
    m_Microphone->seek(0);
    m_Microphone->start();
}

void Widget::restartAudioStream()
{
    collectMicData = true;
    m_Microphone->seek(0);
    // m_Microphone->stop();
    // m_Microphone->start();
    // m_audioSource->stop();
    qDebug()<<" restartAudioStream() |  m_pullMode  "<<m_pullMode;
    m_audioSource->start(m_Microphone.get());
}

void Widget::startSound()
{
    qDebug() << ">>>>>>>>>>>>>>is Main Thread in startSound: " << QThread::isMainThread();
    m_Speaker->start();
    m_audioOutput->stop();
    m_audioOutput->start(m_Speaker.data());
    qDebug() << ">>>>>>>>>>>>>>>>is Main Thread after start: " << QThread::isMainThread();
}

void Widget::stopSound()
{
    qDebug() << "in stop sound...";
    m_audioSource->stop();
    m_Microphone->stop();
    m_audioOutput->stop();
    m_Speaker->stop();

    MicThread.exit();
    qDebug() << "exiting stop sound...";
}

void Widget::on_btnStop_clicked()
{
    stopSound();
}

void Widget::on_btnStart_clicked()
{
    qDebug() << "starting";
    curLessonInt = currentlesson.toInt();
    if (currentlesson != "1")
    {
        ui->lb_review->setText("REVIEW");
        curLessonInt -= 1;
        currentlesson = QString::number(curLessonInt);
    }
    else
    {
        ui->lb_review->setText("CURRENT");
    }
    QString temp = "Lesson #" + currentlesson + " Key " + gKey[curLessonInt-1]
                   + " Test Notes " + gTestGroup[curLessonInt-1];
    ui->lb_title->setText(temp);

    qDebug() << "starting to build array set";

    // get sound array set
    tonicNote = tonic_map[gNote[curLessonInt-1]];
    qDebug() << tonicNote;
    qDebug() << gNote[curLessonInt-1];
    FileLoader files;
    files.GetFileList(tonicNote);
    nPos = 0;
    m_Speaker->moveToThread(&SpeakerThread);
    SpeakerThread.start();
    m_Microphone->moveToThread(&MicThread);
    MicThread.start();
    do_Orientation(nPos);
    nPos++;
}

void Widget::do_Orientation(int nPos)
{
    m_Speaker->newTest( rawRecArrays[orientation[nPos] - 1]);
    qDebug() << "orientation value: " << orientation[nPos] - 1;
    playedNote = orientation[nPos] - 1;
    kbPlayedNote = playedNote + tonicNote;
    m_Speaker->start();
    m_audioOutput->stop();
    m_audioOutput->start(m_Speaker.data());
    MicThread.start();
}

void Widget::do_Quiz(int)
{
    m_Speaker->newTest( rawRecArrays[testNotes[nPos]]);
    qDebug() << "testNotes value: " << testNotes[nPos];
    playedNote = testNotes[nPos] - 1;
    kbPlayedNote = playedNote + tonicNote;
    m_Speaker->start();
    m_audioOutput->stop();
    m_audioOutput->start(m_Speaker.data());
    MicThread.start();
}

void Widget::on_btnNext_clicked()
{

}

