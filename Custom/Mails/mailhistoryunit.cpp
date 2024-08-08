#include "mailhistoryunit.h"
#include "Custom/Mails/ui_mailhistoryunit.h"
#include "mailhistoryunit.h"

MailHistoryUnit::MailHistoryUnit(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MailHistoryUnit)
{
    ui->setupUi(this);
    this->installEventFilter(this);
}

MailHistoryUnit::MailHistoryUnit(const QString& sender, const QString& recipient, const QString& subject, const QString& letterBody, QWidget *parent)
    : MailHistoryUnit(parent)
{
    ui->SenderEmailLabel->setText(recipient);
    ui->LetterSubjectLabel->setText(subject);
    ui->LetterBodyLabel->setText(letterBody);

    m_related_letters.emplace_back(sender, recipient, QDate::currentDate(), subject, letterBody);
}

MailHistoryUnit::MailHistoryUnit(const LetterStruct& Letter, QWidget *parent)
    : MailHistoryUnit(parent)
{
    ui->SenderEmailLabel->setText(Letter.recipient);
    ui->LetterSubjectLabel->setText(Letter.subject);
    ui->LetterBodyLabel->setText(Letter.body);

    m_related_letters.append(Letter);
}

MailHistoryUnit::MailHistoryUnit(const QVector<LetterStruct> &Letters, QWidget *parent) : MailHistoryUnit(parent)
{
    LetterStruct LatestLetter = Letters.constLast();
    ui->SenderEmailLabel->setText(LatestLetter.recipient);
    ui->LetterSubjectLabel->setText(LatestLetter.subject);
    ui->LetterBodyLabel->setText(LatestLetter.body);

    m_related_letters.append(Letters);
}

bool MailHistoryUnit::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == this && event->type() == QEvent::MouseButtonRelease)
    {
        emit OnMouseReleased(m_related_letters);
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

MailHistoryUnit::~MailHistoryUnit()
{
    delete ui;
}
