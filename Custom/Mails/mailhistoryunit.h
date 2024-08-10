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

    LetterStruct(const QString& _sender, const QString& _recipient, const QDate& _timestamp, const QString& _subject, const QString& _body, const QVector<QString> _files_paths)
        : sender(_sender), recipient(_recipient), timestamp(_timestamp), subject(_subject), body(_body), files_paths(_files_paths) {}

    QString sender{};
    QString recipient{};
    QDate timestamp{};
    QString subject{};
    QString body{};
    QVector<QString> files_paths{};

    inline QString GenerateFileName() const
    {
        return QString(sender + "_" + recipient + "_" + timestamp.toString("dd-MM-yyyy") + ".txt");
    }

    operator QString() const
    {
        QString attached_files{};
        if (!files_paths.isEmpty())
        {
            attached_files = "\n\nAttached files: \n";
            attached_files += files_paths.join(";\n");
        }
        return "From: " + sender + "\nTo: " + recipient + "\nSent at: " + timestamp.toString()
               + "\nSubject: " + subject + "\n\n" + body + attached_files;
    }

    friend QDataStream &operator<<(QDataStream &out, const LetterStruct &myStruct)
    {
        QString files_representation = myStruct.files_paths.join("; ");
        out << myStruct.sender << myStruct.recipient << myStruct.timestamp << myStruct.subject << myStruct.body << files_representation;
        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, LetterStruct &myStruct)
    {
        QString files_representation{};

        in >> myStruct.sender >> myStruct.recipient >> myStruct.timestamp >> myStruct.subject >> myStruct.body >> files_representation;

        QStringList files_list = files_representation.split("; ");
        myStruct.files_paths.append(files_list);
        return in;
    }
};

class MailHistoryUnit : public QWidget
{
    Q_OBJECT

public:
    explicit MailHistoryUnit(QWidget *parent = nullptr);
    MailHistoryUnit(const QString& Sender, const QString& Recipient, const QString& Subject, const QString& LetterBody, const QVector<QString> files_paths, QWidget *parent = nullptr);
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
