#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStringList>
#include <QRegularExpression>
#include <QScrollBar>
#include "Custom/Mails/mailhistoryunit.h"
#include "SmtpClient.h"
#include "MailMessageBuilder.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    PopulateMailsHistory();

    connect(ui->N_AttachFileButton, SIGNAL(released()), this, SLOT(SelectLetters_Slot()));
    connect(ui->R_AttachFileButton, SIGNAL(released()), this, SLOT(SelectLetters_Slot()));
}

MainWindow::MainWindow(QWidget* parent, std::shared_ptr<ISXSC::SmtpClient> smtp_client)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_smtp_client(smtp_client)
{
    ui->setupUi(this);

    PopulateMailsHistory();

    connect(ui->N_AttachFileButton, SIGNAL(released()), this, SLOT(SelectLetters_Slot()));
    connect(ui->R_AttachFileButton, SIGNAL(released()), this, SLOT(SelectLetters_Slot()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_LogInButton_released()
{
    m_current_user = ui->LoginLine->text();
    m_current_password = ui->PasswordLine->text();
    m_current_server = ui->ServerLine->text();

    qsizetype colon_index = m_current_server.indexOf(':');
    try
    {
        if (colon_index == -1)
            throw std::runtime_error("Invalid server address. Please provide the port number as well.");

        auto server_part = m_current_server.toStdString().substr(0, colon_index);
        auto port_part = m_current_server.toStdString().substr(colon_index + 1);

        m_smtp_client.lock()->AsyncConnect(server_part, stoi(port_part)).get();
        m_smtp_client.lock()->AsyncAuthenticate(m_current_user.toStdString(), m_current_password.toStdString()).get();

        ui->MainPagesStack->setCurrentIndex((int)EMainPagesIndex::MainPage);
        ui->UserEmailLabel->setText(m_current_user);
    }
    catch (const std::exception& e)
    {
        m_smtp_client.lock()->Reset();
        QMessageBox messageBox;
        messageBox.critical(0,"Error",e.what());
        messageBox.setFixedSize(500,200);
        messageBox.show();
    }
}

void MainWindow::on_EmailLine_editingFinished()
{
    CheckEmails(ui->EmailLine);
}

void MainWindow::on_SendButton_released()
{
    ISXMM::MailMessageBuilder builder;
    builder.set_from(m_current_user.toStdString());
    static QRegularExpression s_split_regex("\\s*,\\s*|\\s+");

    if (!CheckEmails(ui->EmailLine)) return;

    QString recipients = ui->EmailLine->text();
    QStringList email_list = recipients.split(s_split_regex, Qt::SkipEmptyParts);

    QString letter_subject = ui->SubjectLine->text();
    builder.set_subject(letter_subject.toStdString());
    QString letter_body = ui->LetterBodyText->toPlainText();
    builder.set_body(letter_body.toStdString());

    for (const QString& recipient : email_list)
    {
        builder.add_to(recipient.toStdString());
        QDate current_date = QDate::currentDate();
        LetterStruct letter(m_current_user, recipient, current_date, letter_subject, letter_body, m_currently_selected_files);

        SendLetter(letter);

        QString file_name = letter.GenerateFileName();
        QString full_file_name = m_temp_file_path + file_name;

        QVector<LetterStruct> letters {letter};
        SpawnNewHistoryUnit(letter);
        WriteLettersToFile(letters, full_file_name);
    }

    for (const QString& file_path : m_currently_selected_files)
    {
        builder.add_attachment(file_path.toStdString());
    }
    try
    {
        m_smtp_client.lock()->AsyncSendMail(builder.Build()).get();
        ui->N_FileNameLabel->setText("");
        ui->R_FileNameLabel->setText("");
    }
    catch (const std::exception& e)
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Error",e.what());
        messageBox.setFixedSize(500,200);
        messageBox.show();
    }

    CleanNewLetterFields();
    ClearSelectedFilesAndRefreshLabels();
}


void MainWindow::on_NewLetterButton_released()
{
    ui->LetterTypeStack->setCurrentIndex((int)ELetterPagesIndex::NewLetterPage);

    CleanNewLetterFields();
}

void MainWindow::on_SendReplyButton_released()
{
    if (m_current_history_unit)
    {
        const QVector<LetterStruct>& current_letters = m_current_history_unit->get_letters();
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
            LetterStruct letter{m_current_user, recipient_email, QDate::currentDate(), "Reply", letter_body, m_currently_selected_files};

            SendLetter(letter);
            AppendLettersToFile(QVector<LetterStruct>{letter}, m_temp_file_path + first_file_name);

            m_current_history_unit->Append(QVector<LetterStruct>{letter});
            ui->LetterHistoryText->setText(m_current_history_unit->GetFullTextRepresentation());

            int max = ui->LetterHistoryText->verticalScrollBar()->maximum();
            ui->LetterHistoryText->verticalScrollBar()->setValue(max);

            ClearSelectedFilesAndRefreshLabels();
        }
    }
}

void MainWindow::SelectLetters_Slot()
{
    SelectFilesAndRefreshLabels();
}

bool MainWindow::isValidEmail(const QString& email)
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

void MainWindow::PopulateMailsHistory()
{
    QDir dir(m_temp_file_path);

    if (!dir.exists())
    {
        qDebug() << "Directory does not exist:" << m_temp_file_path;
    }

    dir.setSorting(QDir::Time | QDir::Reversed);
    dir.setFilter(QDir::Files);
    QStringList files = dir.entryList();

    for (const QString &fileName : files)
    {
        qDebug() << fileName;
        QVector<LetterStruct> letters = ReadLettersFromFile(m_temp_file_path + fileName);
        if (!letters.isEmpty()) SpawnNewHistoryUnit(letters);
    }
}

void MainWindow::SendLetter(const LetterStruct& Letter)
{
}

bool MainWindow::WriteLettersToFile(const QVector<LetterStruct>& letters, const QString& full_file_name)
{
    QFile file(full_file_name);
    if (!file.open(QIODevice::ReadWrite))
    {
        qDebug() << "Could not open file for writing";
        return false;
    }
    if (letters.isEmpty())
    {
        qDebug() << "Trying to write 0 letters!";
        return false;
    }

    qDebug() << "Writing to " << full_file_name << " " << letters.size() << " letters";

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_6_4);

    out << letters.size();

    for (const auto& letter : letters)
    {
        out << letter;
    }

    file.close();

    return true;
}

QVector<LetterStruct> MainWindow::ReadLettersFromFile(const QString& full_file_name)
{
    QFile file(full_file_name);
    QVector<LetterStruct> Letters;

    if (!QFile::exists(full_file_name))
    {
        qDebug() << "The file " << full_file_name << " was not found";
        return Letters;
    }
    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "Could not open file " << full_file_name << " for reading";
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

void MainWindow::SelectFilesAndRefreshLabels()
{
    m_currently_selected_files = QFileDialog::getOpenFileNames(nullptr, "Select Files", "", "All Files (*.*)");
    QStringList file_names;
    if (!m_currently_selected_files.isEmpty())
    {
        for (const auto& file_path : m_currently_selected_files)
        {
            QFileInfo file_info(file_path);
            QString file_name = file_info.fileName();
            file_names.append(file_name);
        }
        QString files = file_names.join("; ");

        // Haven't found a way to not just copy and paste for each label
        ui->N_FileNameLabel->setText(files);
        ui->R_FileNameLabel->setText(files);
    }
}

void MainWindow::ClearSelectedFilesAndRefreshLabels()
{
    m_currently_selected_files.clear();
    ui->N_FileNameLabel->setText("");
    ui->R_FileNameLabel->setText("");
}

void MainWindow::HistoryWidgetClicked(QVector<LetterStruct> related_letters)
{
    if (!related_letters.isEmpty())
    {
        m_current_history_unit = qobject_cast<MailHistoryUnit*>(sender());
        if (m_current_history_unit)
        {
            ui->LetterHistoryText->setText(m_current_history_unit->GetFullTextRepresentation());
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

template<typename... Args>
void MainWindow::SpawnNewHistoryUnit(Args&&... args)
{
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(ui->MailHistoryScrollArea->layout());
    MailHistoryUnit* new_history_unit = nullptr;

    if (layout)
    {
        new_history_unit = new MailHistoryUnit(std::forward<Args>(args)...);
        layout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
        layout->insertWidget(0, new_history_unit);
    }

    if (new_history_unit)
    {
        connect(new_history_unit, SIGNAL(OnMouseReleased(QVector<LetterStruct>)), this, SLOT(HistoryWidgetClicked(QVector<LetterStruct>)));
    }
}
