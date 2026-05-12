#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QByteArray>
#include <QAudioSource>
#include <QAudioSink>
#include <QIODevice>
#include <QMediaDevices>
#include <QScopedPointer>

class Microphone : public QIODevice
{
    Q_OBJECT

public:
    Microphone(const QAudioFormat &format);

    void start();
    void stop();

    qreal level() const { return m_level; }

    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

    qreal getNoteValue(const char *data, qint64 len) const;

signals:
    void levelChanged(qreal level);
    void haltstream();

private:
    const QAudioFormat m_format;
    qreal m_level = 0.0; // 0.0 <= m_level <= 1.0
};

class Speaker : public QIODevice
{
    Q_OBJECT

public:
    Speaker();
    void start();
    void stop();
    void getWaveFile(QString noteFile);
    void newTest(QByteArray bufferOut);
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;
    qint64 bytesAvailable() const override;
    qint64 size() const override { return m_buffer.size(); }
    qint64 m_pos = 0;
    QByteArray m_buffer;
};


QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget() override;
    void do_Orientation(int);
    void do_Quiz(int);
    int nPos;
    int tonicNote;
    int playedNote;
    int kbPlayedNote;
    int heardNote;
    bool orientationFlag;
    bool timeDelay;
    QMediaDevices *m_devicesOut = nullptr;
    QScopedPointer<Microphone> m_Microphone;
    QScopedPointer<QAudioSource> m_audioSource;
    bool m_pullMode = false;

public slots:
    void micFoundNote(int value);
    void updateKBnote(int kbValue, float acc);

protected:
    void paintEvent(QPaintEvent *event);

private slots:
    void on_btnStart_clicked();
    void startSound();
    void stopSound();
    void on_btnStop_clicked();
    void on_btnNext_clicked();

private:
    void initializeAudioOutput(const QAudioDevice &deviceInfo);
    void initializeAudioInput(const QAudioDevice &deviceInfo);
    void restartAudioStream();

signals:
    void startSpeaker();

private:
    QMediaDevices *m_devicesIn = nullptr;
    QScopedPointer<Speaker> m_Speaker;
    QScopedPointer<QAudioSink> m_audioOutput;

private:
    Ui::Widget *ui;
};
#endif // WIDGET_H
