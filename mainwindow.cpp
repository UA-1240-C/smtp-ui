#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStringList>
#include <QRegularExpression>
#include <QScrollBar>
#include "Custom/Mails/mailhistoryunit.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    PopulateMailsHistory();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_LogInButton_released()
{
    ui->MainPagesStack->setCurrentIndex((int)EMainPagesIndex::MainPage);
}

void MainWindow::on_AttachFileButton_released()
{
    QStringList file_paths = QFileDialog::getOpenFileNames(nullptr, "Select Files", "", "All Files (*.*)");
    QStringList file_names;
    if (!file_paths.isEmpty())
    {
        for (const auto& file_path : file_paths)
        {
            QFileInfo file_info(file_path);
            QString file_name = file_info.fileName();
            file_names.append(file_name);
        }
        QString files = file_names.join("; ");
        ui->FileNameLabel->setText(files);
    }
}

bool MainWindow::isValidEmail(const QString &email)
{
    static QRegularExpression s_email_regex("^[\\w\\.-]+@[\\w\\.-]+\\.\\w+$");

    QRegularExpressionMatch match = s_email_regex.match(email);
    return match.hasMatch();
}


bool MainWindow::CheckEmails(const QLineEdit* Container)
{
    static QRegularExpression s_split_regex("\\s*,\\s*|\\s+");

    QString recipients = Container->text();
    QStringList email_list = recipients.split(s_split_regex, Qt::SkipEmptyParts);
    if (email_list.isEmpty())
    {
        QMessageBox::critical(this, "No recipients!", "You haven't written any recipients!");
        return false;
    }

    QStringList invalid_emails;

    for (const QString &email : email_list)
    {
        if (!isValidEmail(email))
        {
            invalid_emails += email;
        }
    }

    if (!invalid_emails.isEmpty())
    {
        QString msg = R"*(Invalid emails: ")*";
        msg += invalid_emails.join("; ");
        msg += R"*("!)*";

        QMessageBox::critical(this, "Invalid email detected!", msg);
        return false;
    }
    return true;
}

void MainWindow::on_EmailLine_editingFinished()
{
    CheckEmails(ui->EmailLine);
}

void MainWindow::SpawnNewHistoryUnit(const LetterStruct& Letter)
{
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(ui->MailHistoryScrollArea->layout());
    MailHistoryUnit* new_history_unit = nullptr;

    if (layout)
    {
        new_history_unit = new MailHistoryUnit(Letter);
        layout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
        layout->insertWidget(0, new_history_unit);
    }

    connect(new_history_unit, SIGNAL(OnMouseReleased(QVector<LetterStruct>)), this, SLOT(HistoryWidgetClicked(QVector<LetterStruct>)));
}

void MainWindow::SpawnNewHistoryUnit(const QVector<LetterStruct>& Letters)
{
    MailHistoryUnit* new_history_unit = new MailHistoryUnit(Letters);
    ui->MailHistoryScrollArea->layout()->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    ui->MailHistoryScrollArea->layout()->addWidget(new_history_unit);

    connect(new_history_unit, SIGNAL(OnMouseReleased(QVector<LetterStruct>)), this, SLOT(HistoryWidgetClicked(QVector<LetterStruct>)));
}

void MainWindow::PopulateMailsHistory()
{
    QDir dir(m_temp_file_path);

    if (!dir.exists())
    {
        qDebug() << "Directory does not exist:" << m_temp_file_path;
    }

    dir.setSorting(QDir::Time);
    dir.setFilter(QDir::Files);
    QStringList files = dir.entryList();

    for (const QString &fileName : files)
    {
        qDebug() << fileName;
        QVector<LetterStruct> letters = ReadLettersFromFile(m_temp_file_path + fileName);
        if (!letters.isEmpty()) SpawnNewHistoryUnit(letters);
    }
}

void MainWindow::on_SendButton_released()
{
    static QRegularExpression s_split_regex("\\s*,\\s*|\\s+");

    if (!CheckEmails(ui->EmailLine)) return;

    QString Recipients = ui->EmailLine->text();
    QStringList EmailList = Recipients.split(s_split_regex, Qt::SkipEmptyParts);

    QString LetterSubject = ui->SubjectLine->text();
    QString LetterBody = ui->LetterBodyText->toPlainText();

    for (const QString& Recipient : EmailList)
    {
        QDate CurrentDate = QDate::currentDate();
        LetterStruct Letter(m_current_user, Recipient, CurrentDate, LetterSubject, LetterBody);

        SendLetter(Letter);

        QString FileName = Letter.GenerateFileName();
        QString FullFileName = m_temp_file_path + FileName;

        QVector<LetterStruct> Letters {Letter};
        SpawnNewHistoryUnit(Letter);
        WriteLettersToFile(Letters, FullFileName);
    }

    CleanNewLetterFields();
}

void MainWindow::SendLetter(const LetterStruct& Letter)
{

}

bool MainWindow::WriteLettersToFile(const QVector<LetterStruct>& Letters, const QString& full_file_name)
{
    QFile file(full_file_name);
    if (!file.open(QIODevice::ReadWrite))
    {
        qDebug() << "Could not open file for writing";
        return false;
    }
    if (Letters.isEmpty())
    {
        qDebug() << "Trying to write 0 letters!";
        return false;
    }

    qDebug() << "Writing to " << full_file_name << " " << Letters.size() << " letters";

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_6_7);

    out << Letters.size();

    for (const auto& Letter : Letters)
    {
        out << Letter;
    }

    file.close();

    return true;
}

QVector<LetterStruct> MainWindow::ReadLettersFromFile(const QString& full_file_name)
{
    QFile file(full_file_name);
    QVector<LetterStruct> Letters;

    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "Could not open file " << full_file_name << " for reading";
        return Letters;
    }
    if (!QFile::exists(full_file_name))
    {
        qDebug() << "The file " << full_file_name << " was not found";
        return Letters;
    }

    qDebug() << "Reading from: " << full_file_name;

    QDataStream in(&file);

    qsizetype size;
    in >> size;
    qDebug() << size;

    for (qsizetype i = 0; i < size; i++)
    {
        LetterStruct Letter;
        in >> Letter;
        Letters.append(Letter);
    }

    file.close();
    return Letters;
}

bool MainWindow::AppendLettersToFile(const QVector<LetterStruct>& Letters, const QString& FullFileName)
{
    QVector<LetterStruct> existing_letters = ReadLettersFromFile(FullFileName);

    existing_letters.append(Letters);

    WriteLettersToFile(existing_letters, FullFileName);

    return true;
}

void MainWindow::HistoryWidgetClicked(QVector<LetterStruct> related_letters)
{
    if (!related_letters.isEmpty())
    {
        m_temp_current_history_unit = qobject_cast<MailHistoryUnit*>(sender());
        if (m_temp_current_history_unit)
        {
            ui->LetterHistoryText->setText(m_temp_current_history_unit->GetFullTextRepresentation());
            ui->LetterTypeStack->setCurrentIndex((int)ELetterPagesIndex::ReplyPage);
        }
    }
}

void MainWindow::CleanNewLetterFields()
{
    ui->EmailLine->setText("");
    ui->SubjectLine->setText("");
    ui->LetterBodyText->setText("");
}

void MainWindow::on_NewLetterButton_released()
{
    ui->LetterTypeStack->setCurrentIndex((int)ELetterPagesIndex::NewLetterPage);

    CleanNewLetterFields();
}


void MainWindow::on_SendReplyButton_released()
{
    if (m_temp_current_history_unit)
    {
        const QVector<LetterStruct>& current_letters = m_temp_current_history_unit->get_letters();
        if (!current_letters.isEmpty())
        {
            QString first_file_name{};
            QString recipient_email{};
            for (const auto& letter : current_letters)
            {
                if (first_file_name.isEmpty())
                {
                    first_file_name = letter.GenerateFileName();
                }

                if (letter.recipient != m_current_user)
                {
                    recipient_email = letter.recipient;
                }
                else
                {
                    break;
                }
            }
            QString letter_body = ui->ReplyLine->text();
            LetterStruct letter{m_current_user, recipient_email, QDate::currentDate(), "Reply", letter_body};

            SendLetter(letter);
            AppendLettersToFile(QVector<LetterStruct>{letter}, m_temp_file_path + first_file_name);

            m_temp_current_history_unit->Append(QVector<LetterStruct>{letter});
            ui->LetterHistoryText->setText(m_temp_current_history_unit->GetFullTextRepresentation());

            int max = ui->LetterHistoryText->verticalScrollBar()->maximum();
            ui->LetterHistoryText->verticalScrollBar()->setValue(max);
        }
    }
}

