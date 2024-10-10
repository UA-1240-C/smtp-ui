#include "mailhistoryunit.h"
#include "Custom/Mails/ui_mailhistoryunit.h"

MailHistoryUnit::MailHistoryUnit(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MailHistoryUnit)
{
    ui->setupUi(this);
    this->installEventFilter(this);
}

MailHistoryUnit::MailHistoryUnit(const QString& sender, const QString& recipient
                                 , const QString& subject, const QString& letterBody
                                 , const QVector<QString>& files_paths, QWidget *parent)
    : MailHistoryUnit(parent)
{
    LetterStruct letter{sender, recipient, QDate::currentDate().toString(), subject, letterBody, files_paths};

    RenewUiInfo(letter);

    m_related_letters.emplace_back(std::move(letter));
}

MailHistoryUnit::MailHistoryUnit(const LetterStruct& Letter, QWidget* parent)
    : MailHistoryUnit(parent)
{
    RenewUiInfo(Letter);

    m_related_letters.append(Letter);
}

MailHistoryUnit::MailHistoryUnit(const QVector<LetterStruct>& Letters, QWidget* parent) : MailHistoryUnit(parent)
{
    LetterStruct latest_letter = Letters.constLast();
    RenewUiInfo(latest_letter);

    m_related_letters.append(Letters);
}

const QVector<LetterStruct>& MailHistoryUnit::get_letters() const
{
    return m_related_letters;
}

QString MailHistoryUnit::GetFullTextRepresentation() const
{
    QString res;
    for (int i = 0; i < m_related_letters.size(); i++)
    {
        res += (const QString)m_related_letters[i];
        if (i != m_related_letters.size() - 1)
        {
            res += "\n--------------------------------\n";
        }
    }
    return res;
}

void MailHistoryUnit::Append(const QVector<LetterStruct>& NewLetters)
{
    m_related_letters.append(NewLetters);
}

bool MailHistoryUnit::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == this && event->type() == QEvent::MouseButtonRelease)
    {
        emit OnMouseReleased(m_related_letters);
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

void MailHistoryUnit::RenewUiInfo(LetterStruct letter)
{
    ui->SenderEmailLabel->setText(letter.recipient);
    ui->LetterSubjectLabel->setText(letter.subject);
    ui->LetterBodyLabel->setText(letter.body);
}

MailHistoryUnit::~MailHistoryUnit()
{
    delete ui;
}
