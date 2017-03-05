#include "iniciarsesion.h"
#include "ui_iniciarsesion.h"
#include "principal.h"
#include <QMessageBox>
#include "administrador/parametro/servidorconfigura.h"
#include <QCompleter>
#include <QDebug>
#include "qdbhelper.h"

IniciarSesion::IniciarSesion(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IniciarSesion)
{
    ui->setupUi(this);


    QCompleter *completer = new QCompleter(getServidores(), this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    ui->txtServidor->setCompleter(completer);

    ui->txtServidor->setText(getServidorPredeterminado());


    connect(ui->pushCancelar,SIGNAL(pressed()),this,SLOT(OnQuit()));
    connect(ui->pushAceptar,SIGNAL(pressed()),this,SLOT(OnLogin()));
}

void IniciarSesion::reject(){
    OnQuit();
}

void IniciarSesion::OnQuit(){
    this->close();
    parentWidget()->close();
}
void IniciarSesion::OnLogin(){
    QString usuario=ui->txtUsuario->text();
    QString clave=ui->txtClave->text();

    if(usuario.isEmpty() || clave.isEmpty()){

        QMessageBox::information(this,tr("Advertencia"),"Usuario o clave, no validos");
    }
    else{
        if (Conectar()){
            this->destroy();
        }
        else{
            QMessageBox::critical(this,tr("Advertencia"),"No se pudo establecer la conexión");
        }
    }

}
//Obtiene el servidor predeterminado de el archivo de configuración
QString IniciarSesion::getServidorPredeterminado(){
    QSettings settings(ServidorConfigura::homeConfig+ QDir::separator() +"Arenita.ini", QSettings::NativeFormat);
    settings.beginGroup("Prioridad");
    QString servidor=settings.value("Servidor").toString();
    settings.endGroup();
    qDebug() << "Servidor predeterminado:" << servidor;
    return servidor;
}
//Obtiene la lista de servidores del archivo de configuración
QStringList IniciarSesion::getServidores(){
    QSettings settings(ServidorConfigura::homeConfig+ QDir::separator() +"Arenita.ini", QSettings::NativeFormat);

    QStringList grupos = settings.childGroups();

    grupos.removeOne("Prioridad");

    foreach (const QString &group, grupos) {
        QString groupString = QString("%1").arg(group);
        settings.beginGroup(group);
        qDebug() << "Grupo:" << groupString;
        foreach (const QString &key, settings.childKeys()) {
            QString nombre;
            QString valor;
            nombre.append(QString("%1").arg(key));
            valor.append(QString("%1").arg(settings.value(key).toString()));
            qDebug() << "  Nombre:" << nombre << " Valor:" << valor;
        }
        qDebug() << "\n";
        settings.endGroup();
    }

    return grupos;
}

IniciarSesion::DatosConexion IniciarSesion::getDatosConexion()
{
    DatosConexion dc;
    QSettings settings(ServidorConfigura::homeConfig+ QDir::separator() +"Arenita.ini", QSettings::NativeFormat);

    settings.beginGroup(ui->txtServidor->text());

    const QStringList servidor = settings.childKeys();
    /*
     * Recupera los datos de conexión en la estructura
    */
    foreach (const QString &nombre, servidor)
    {
        if(nombre=="ip"){
            dc.ip=settings.value(nombre).toString();
        }
        else if (nombre=="puerto") {
            dc.puerto=settings.value(nombre).toString();
        }
        else if (nombre=="baseDatos") {
            dc.baseDatos=settings.value(nombre).toString();
        }
    }
    settings.endGroup();
    /*
     * Añade el usuario y la clave en la estructura
    */
    dc.usuario=ui->txtUsuario->text();
    dc.clave=ui->txtClave->text();

    qDebug() << "Datos de Conexión";
    qDebug() << "ip:" << dc.ip;
    qDebug() << "puerto:" << dc.puerto;
    qDebug() << "base de datos:" << dc.baseDatos;
    //qDebug() << "usuario:" << datosConexion.usuario;
    //qDebug() << "clave:" << datosConexion.clave;

    return dc;
}

IniciarSesion::~IniciarSesion()
{
    delete ui;
}


bool IniciarSesion::Conectar()
{

    const char* driverName = "QPSQL";
    QdbHelper* qdbHelper = new QdbHelper(driverName);


    DatosConexion dc=getDatosConexion();

    QSqlDatabase* db = qdbHelper->connect(dc.ip, dc.puerto, dc.baseDatos, dc.usuario, dc.clave);

    QString tipo;
    QString estado;
    bool exists = false;
    if(db){
        QSqlQuery checkQuery(*db);
        checkQuery.prepare("select tipo,estado from seg_usuarios where usuario=(:u)");
        checkQuery.bindValue(":u", dc.usuario);

        if (checkQuery.exec())
        {
            if (checkQuery.next())
            {
                exists = true;
                tipo = checkQuery.value("tipo").toString();
                qDebug() << "Tipo" << tipo;
                estado = checkQuery.value("estado").toString();
                qDebug() << "Estado" << estado;
                if(estado=="Inactivo"){
                    QMessageBox::warning(this,tr("Advertencia"),"El usuario está inactivo");
                    exists = false;
                }
            }
            else{
                QMessageBox::warning(this,tr("Advertencia"),"El usuario no está registrado en la base de datos");
            }
        }
        qDebug() << "Existe" << exists;

    }
    if(exists){
        datosConexion=dc;
        datosConexion.tipo=tipo;
    }
    return exists;
}

void IniciarSesion::on_pushConexiones_clicked()
{
    ServidorConfigura* configura=new ServidorConfigura();
    configura->showMaximized();
}
