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

    LetterStruct(const QString& _sender, const QString& _recipient, const QDate& _timestamp, const QString& _subject, const QString& _body)
        : sender(_sender), recipient(_recipient), timestamp(_timestamp), subject(_subject), body(_body) {}

    QString sender{};
    QString recipient{};
    QDate timestamp{};
    QString subject{};
    QString body{};

    inline QString GenerateFileName() const
    {
        return QString(sender + "_" + recipient + "_" + timestamp.toString("dd-MM-yyyy") + ".txt");
    }

    operator QString() const
    {
        return "From: " + sender + "\nTo: " + recipient + "\nSent at: " + timestamp.toString() + "\nSubject: " + subject + "\n\n" + body;
    }

    friend QDataStream &operator<<(QDataStream &out, const LetterStruct &myStruct)
    {
        out << myStruct.sender << myStruct.recipient << myStruct.timestamp << myStruct.subject << myStruct.body;
        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, LetterStruct &myStruct)
    {
        in >> myStruct.sender >> myStruct.recipient >> myStruct.timestamp >> myStruct.subject >> myStruct.body;
        return in;
    }
};

class MailHistoryUnit : public QWidget
{
    Q_OBJECT

public:
    explicit MailHistoryUnit(QWidget *parent = nullptr);
    MailHistoryUnit(const QString& Sender, const QString& Recipient, const QString& Subject, const QString& LetterBody, QWidget *parent = nullptr);
    MailHistoryUnit(const LetterStruct& Letter, QWidget *parent = nullptr);
    MailHistoryUnit(const QVector<LetterStruct>& Letters, QWidget *parent = nullptr);
    ~MailHistoryUnit();

    const QVector<LetterStruct>& get_letters() const;

    QString GetFullTextRepresentation() const;

    void Append(const QVector<LetterStruct>& NewLetters);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::MailHistoryUnit *ui;
    QVector<LetterStruct> m_related_letters;

signals:
    void OnMouseReleased(QVector<LetterStruct> RelatedLetters);
};

#endif // MAILHISTORYUNIT_H
