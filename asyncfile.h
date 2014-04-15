#ifndef ASYNCFILE_H
#define ASYNCFILE_H

#include <QIODevice>

#ifdef Q_OS_WIN
#  include <Windows.h>
#endif

#include <QFileDevice>

#define BUFFER_SIZE 1024
#define MAX_BUFFER_SIZE 10*1024

class AsyncFile : public QIODevice
{
    Q_OBJECT
public:
    explicit AsyncFile(QObject *parent = 0);
    explicit AsyncFile(const QString &path, QObject *parent = 0);

    qint64 bytesAvailable() const;

    bool open(OpenMode mode);

    qint64 size() const;

    QFileDevice::FileError error() const;

    bool waitForReadyRead(int msecs);

protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 maxlen);

private:
    void setError(QFileDevice::FileError error);
    void setError(QFileDevice::FileError error, const QString &message);
    void readFinished(qint64 bytes);

private:
    QFileDevice::FileError m_error;
    QString m_filePath;
    QByteArray m_readBuffer;

#ifdef Q_OS_WIN
private slots:
    bool asyncRead(qint64 pos);
private:
    static void readCallback(DWORD errorCode, DWORD numberOfBytesTransfered, LPOVERLAPPED overlapped);

    struct MyOverlapped
    {
        OVERLAPPED overlapped;
        AsyncFile *that;

        MyOverlapped(AsyncFile *that) :
            that(that)
        {
            Q_ASSERT((void *)&overlapped == (void *)this);
            memset(&overlapped, 0, sizeof(OVERLAPPED));
            overlapped.hEvent = CreateEvent(0,TRUE,FALSE,0);
        }
        char buffer[BUFFER_SIZE];
    };

    HANDLE m_FileHandle;
    MyOverlapped overlapped;
    bool reading;

#endif
};

#endif // ASYNCFILE_H
