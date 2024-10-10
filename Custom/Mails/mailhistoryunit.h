#ifndef MAILHISTORYUNIT_H
#define MAILHISTORYUNIT_H

#include <QWidget>
#include <qdatetime.h>

namespace Ui {
class MailHistoryUnit;
}

struct LetterStruct
{
    LetterStruct(){}

    LetterStruct(const QString& _sender, const QString& _recipient
                 , const QString& _timestamp, const QString& _subject
                 , const QString& _body, const QVector<QString>& _files_paths)
        : sender(_sender)
        , recipient(_recipient)
        , timestamp(_timestamp)
        , subject(_subject)
        , body(_body)
        , files_paths(_files_paths)
    {}

    inline QString GenerateFileName() const
    {
        return QString(sender + "_" + recipient + "_" + timestamp + ".txt");
    }

    operator QString() const
    {
        QString attached_files{};
        if (!files_paths.isEmpty())
        {
            attached_files = "\n\nAttached files: \n";
            attached_files += files_paths.join(";\n");
        }
        return "From: " + sender + "\nTo: " + recipient + "\nSent at: " + timestamp
               + "\nSubject: " + subject + "\n\n" + body + attached_files;
    }

    QString sender{};
    QString recipient{};
    QString timestamp{};
    QString subject{};
    QString body{};
    QVector<QString> files_paths{};
};

class MailHistoryUnit : public QWidget
{
    Q_OBJECT

public:
    explicit MailHistoryUnit(QWidget* parent = nullptr);
    MailHistoryUnit(const QString& Sender, const QString& Recipient
                    , const QString& Subject, const QString& LetterBody
                    , const QVector<QString>& files_paths, QWidget* parent = nullptr);
    MailHistoryUnit(const LetterStruct& Letter, QWidget* parent = nullptr);
    MailHistoryUnit(const QVector<LetterStruct>& Letters, QWidget* parent = nullptr);
    ~MailHistoryUnit();

    const QVector<LetterStruct>& get_letters() const;

    QString GetFullTextRepresentation() const;

    void Append(const QVector<LetterStruct>& NewLetters);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void RenewUiInfo(LetterStruct letter);

    Ui::MailHistoryUnit* ui;
    QVector<LetterStruct> m_related_letters;

signals:
    void OnMouseReleased(QVector<LetterStruct> RelatedLetters);
};

#endif // MAILHISTORYUNIT_H
