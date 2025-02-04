# -*- coding: utf-8 -*-
from PyQt5.QtCore import Qt, QRegExp, QThread
from PyQt5.QtGui import QPainter, QPen, QPixmap, QRegExpValidator
from PyQt5.QtWidgets import QMainWindow, QApplication, QFileDialog, QMessageBox, QPushButton
from PyQt5 import uic
from PyQt5 import QtCore, QtWidgets
from functools import partial
import PyQt5
import sys
import time
import version
import modelador as md
import interfazAG
import subprocess 
import traceback



form_class = uic.loadUiType("interfaz.ui")[0]  # Load the UI
isDev = None#1

class Proceso():
    def __init__(self, cajasPallet, costoProduccion, demandaCliente, familiaProductos, maestroMateriales, nodos, precioVenta, tarifas, stock, sets, archCuadratura, archRutas, fecha, archIndicadores, carpSalida, tiempoLimite, archDetalle, conCuadratura, compartidos, prodNoConsolida, licencia):
        self.cajasPallet = cajasPallet
        self.costoProduccion = costoProduccion
        self.demandaCliente = demandaCliente
        self.familiaProductos = familiaProductos
        self.maestroMateriales = maestroMateriales
        self.nodos = nodos
        self.precioVenta = precioVenta
        self.tarifas = tarifas
        self.stock = stock
        self.sets = sets
        self.archCuadratura = archCuadratura
        self.archRutas = archRutas
        self.fecha = fecha
        self.archIndicadores = archIndicadores
        self.carpSalida = carpSalida
        self.tiempoLimite = tiempoLimite
        self.archDetalle = archDetalle
        self.conCuadratura = conCuadratura
        self.compartidos = compartidos
        self.prodNoConsolida = prodNoConsolida
        self.mod = None
        self.excepcion = None
        self.licencia = licencia

    def paso1(self):
        try:
            self.mod = md.Modelador(
                self.cajasPallet,
                self.costoProduccion,
                self.demandaCliente,
                self.familiaProductos,
                self.maestroMateriales,
                self.nodos,
                self.precioVenta,
                self.tarifas,
                self.stock,
                self.sets,
                self.archCuadratura,
                self.archRutas,
                self.fecha,
                self.archIndicadores,
                self.carpSalida,
                self.tiempoLimite,
                self.archDetalle,
                self.conCuadratura,
                self.compartidos,
                self.prodNoConsolida,
                self.licencia
            )
        except Exception as e:
            print(traceback.format_exc())
            self.excepcion = str(e)
            
    def paso2(self):
        try:
            self.mod.ejecutar()
        except Exception as e:
            print(traceback.format_exc())
            self.excepcion = str(e)

    def paso3(self):
        try:
            if self.mod.tieneSolucion:
                #################################
                ## Sección C++
                #################################
                cmd = [
                    "DFS.exe", 
                    self.carpSalida + "/logistica/tmp/testout-v13largo.csv",
                    self.carpSalida + "/logistica/tmp/testout-V13cuad.csv",
                    self.carpSalida + "/logistica/tmp",
                    str(int(self.conCuadratura)),
                    self.carpSalida + "/logistica/tmp/exportCPP.txt"
                    ]
                try:
                    proc = subprocess.check_output(cmd, stderr=subprocess.PIPE, shell=True)

#                proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
#                proc.wait()
#                (stdout, stderr) = proc.communicate()
#
#                for line in str(stdout)[2:-1].replace("\\r", "").split("\\n"):
#                    print(line)
#
#                if proc.returncode != 0:
#                    raise ValueError("[C++] Error de ejecición. " + stderr);

                    for line in str(proc)[2:-1].replace("\\r", "").split("\\n"):
                        print(line)
                except subprocess.CalledProcessError as e:
                    print('{}'.format(e.output.decode(sys.getfilesystemencoding())))
                    print("##############################################################")
                    print('{}'.format(e.stderr.decode(sys.getfilesystemencoding())))
                    raise ValueError("[C++] Error de ejecición. " + e.stderr.decode(sys.getfilesystemencoding()));
            else:
                raise ValueError("El modelo no tiene solución.");

            self.mod.grabarRutas() 
            ######self.mod.grabarCuadratura()
            self.mod.grabarIndicadores()
            self.mod.grabarDetalle()
            self.mod.grabarExportSAP()
            self.mod.grabarZlogTransportes()
            self.mod.grabarZlogSuministros()
        except Exception as e:
            print(traceback.format_exc())
            self.excepcion = str(e)

    def paso4(self):
        try:
            interfazSilenciosa = interfazAG.InterfazSilenciosa() 
            interfazSilenciosa.run(
                self.mod.pathTmpLogistica,
                self.maestroMateriales,
                self.nodos,
                self.tarifas,
                self.carpSalida + "/agendamiento",
                self.tiempoLimite,
                self.fecha,
                self.mod.env
                )

        except Exception as e:
            print(traceback.format_exc())
            self.excepcion = str(e)


class PThread(QThread):
    def __init__(self, pasos, paso):
        super().__init__()
        self.pasos = pasos
        self.paso = paso

    def run(self):
        self.pasos[self.paso]()
        self.pasos = None


class Interfaz(QMainWindow, form_class):
    def __init__(self, parent=None):
        QMainWindow.__init__(self, parent)
        self.setupUi(self)
        self.setWindowTitle(self.windowTitle() + " (VER." + version.val + ")")
        for row in range(0, self.gridLayout.rowCount()):
            btn = self.gridLayout.itemAtPosition(row, 2).widget()
            if isinstance(btn, PyQt5.QtWidgets.QToolButton):
                btn.clicked.connect(partial(self.seleccionarArchvio, row))
        self.toolButton_13.clicked.connect(self.seleccionarCarpeta)
        self.toolButton_14.clicked.connect(partial(self.seleccionarArchvioEnv, 0))
        
        self.dateEdit.setDateTime(QtCore.QDateTime.currentDateTime())
        self.frame.setHidden(True)
        self.frame_2.setHidden(True)
        self.pixmap = QPixmap("aceptar.png")
        self.pixmap = self.pixmap.scaled(12, 12, QtCore.Qt.KeepAspectRatio)

        # Comprobador de caracteres y números
        reg = QRegExp('[a-zA-Z_][a-zA-Z_0-9]+$')
        # Crea un validador
        validator = QRegExpValidator(self)
        validator.setRegExp(reg)
        self.lineEdit_14.setValidator(validator)
        self.lineEdit_15.setValidator(validator)
        self.lineEdit_16.setValidator(validator)
        
        if isDev:
            self.lineEdit_5.setText("Abastecimiento/Costo_29.01.2025.xlsx")
            self.lineEdit_4.setText("Abastecimiento/Abastecimiento_29.01.2025.xlsx")
            self.lineEdit_3.setText("Abastecimiento/Precio_29.01.2025.xlsx")
            self.lineEdit_2.setText("Abastecimiento/Stock_29.01.2025.xlsx")
            self.lineEdit.setText("Abastecimiento/Set_29.01.2025.xlsx")
            
        
    def seleccionarArchvio(self, row):
        if not row is None:
            options = QFileDialog.Options()
            fileName, _ = QFileDialog.getOpenFileName(self,"Seleccione Archivo", "*.xlsx","Archivos excel (*.xlsx)", options=options)
            if fileName:
                self.gridLayout.itemAtPosition(row, 1).widget().setText(fileName)
    
    def seleccionarArchvioEnv(self, row):
        if not row is None:
            options = QFileDialog.Options()
            fileName, _ = QFileDialog.getOpenFileName(self,"Seleccione Archivo", "*.env","Archivos de entorno (*.env)", options=options)
            if fileName:
                self.gridLayout_3.itemAtPosition(row, 1).widget().setText(fileName)
    
    def seleccionarCarpeta(self):
        carpeta = str(QFileDialog.getExistingDirectory(self, "Seleccione Carpeta"))
        if carpeta:
            self.lineEdit_13.setText(carpeta)
    
    
    def validarFormulario(self):
        isVal = True
        detalle = ""
        for row in range(0, self.gridLayout.rowCount()):
            qle = self.gridLayout.itemAtPosition(row, 1).widget()
            if isinstance(qle, PyQt5.QtWidgets.QLineEdit):
                if qle.text() == "":
                    qle.setStyleSheet("border: 1px solid red;")
                    isVal = False
                else:
                    qle.setStyleSheet("border: 1px solid green;")
        if self.lineEdit_13.text() == "":
            self.lineEdit_13.setStyleSheet("border: 1px solid red;")
            isVal = False
        else:
            self.lineEdit_13.setStyleSheet("border: 1px solid green;")
        if self.lineEdit_14.text() == "":
            self.lineEdit_14.setStyleSheet("border: 1px solid red;")
            isVal = False
        else:
            self.lineEdit_14.setStyleSheet("border: 1px solid green;")
        if self.lineEdit_15.text() == "":
            self.lineEdit_15.setStyleSheet("border: 1px solid red;")
            isVal = False
        else:
            self.lineEdit_15.setStyleSheet("border: 1px solid green;")
        if self.lineEdit_16.text() == "":
            self.lineEdit_16.setStyleSheet("border: 1px solid red;")
            isVal = False
        else:
            self.lineEdit_16.setStyleSheet("border: 1px solid green;")

         
        if not isVal:
            detalle = "\t - El formulario tiene campos vacios."
        
        tdate = self.dateEdit.date() 
        fechaXLSX = '_{:02d}.{:02d}.{}.xlsx'.format(tdate.day(), tdate.month(), tdate.year())        
        fechaError = []
        if not self.lineEdit_2.text().endswith(fechaXLSX):
            fechaError.append("Stock")
            self.lineEdit_2.setStyleSheet("border: 1px solid red;")
            isVal = False
        else:
            self.lineEdit_2.setStyleSheet("border: 1px solid green;")
        if not self.lineEdit_4.text().endswith(fechaXLSX):
            fechaError.append("Demanda")
            self.lineEdit_4.setStyleSheet("border: 1px solid red;")
            isVal = False
        else:
            self.lineEdit_4.setStyleSheet("border: 1px solid green;")
        if not self.lineEdit.text().endswith(fechaXLSX):
            fechaError.append("Set")
            self.lineEdit.setStyleSheet("border: 1px solid red;")
            isVal = False
        else:
            self.lineEdit.setStyleSheet("border: 1px solid green;")
        if len(fechaError) > 0:
            detalle = detalle + "\n\t - El o Los nombre(s) de lo(s) archivo(s) de " + ', '.join(fechaError) + " deben terminar con la fecha seleccionada en el parámetro Fecha: " + fechaXLSX
              
        return isVal, detalle
    
    def reiniciarDetalleProgreso(self):
        self.frame.setHidden(False)
        self.frame_2.setHidden(False)
        self.progressBar.setValue(0)
        self.label_23.setStyleSheet("color: grey;border-width: 0px 0px 0px 0px;font-weight: bold;")
        self.label_24.setStyleSheet("color: lightgrey;border-width: 0px 0px 0px 0px;")
        self.label_25.setStyleSheet("color: lightgrey;border-width: 0px 0px 0px 0px;")
        self.label_26.setStyleSheet("color: lightgrey;border-width: 0px 0px 0px 0px;")
        self.label_27.clear()
        self.label_28.clear()
        self.label_29.clear()
        self.label_30.clear()
        self.repaint()
    
    def actualizarProgreso(self, avance):
        if avance == 25:
            self.label_23.setStyleSheet("color: green;border-width: 0px 0px 0px 0px;font-weight: bold;font-size: 12px;")
            self.label_24.setStyleSheet("color: grey;border-width: 0px 0px 0px 0px;font-weight: bold;")
            self.label_27.setPixmap(self.pixmap)
        if avance == 50:
            self.label_24.setStyleSheet("color: green;border-width: 0px 0px 0px 0px;font-weight: bold;font-size: 12px;")
            self.label_25.setStyleSheet("color: grey;border-width: 0px 0px 0px 0px;font-weight: bold;")
            self.label_28.setPixmap(self.pixmap)
        if avance == 75:
            self.label_25.setStyleSheet("color: green;border-width: 0px 0px 0px 0px;font-weight: bold;font-size: 12px;")
            self.label_26.setStyleSheet("color: grey;border-width: 0px 0px 0px 0px;font-weight: bold;")
            self.label_29.setPixmap(self.pixmap)
        if avance == 100:
            self.label_26.setStyleSheet("color: green;border-width: 0px 0px 0px 0px;font-weight: bold;font-size: 12px;")
            self.label_30.setPixmap(self.pixmap)
        self.progressBar.setValue(avance)
        self.repaint()
        
    def procesar(self):
        isVal, detalle = self.validarFormulario()
        if isVal:
            self.reiniciarDetalleProgreso()
            print("\n[-------------- EJECUTANDO ------------------]")
            self.proceso = Proceso(
                self.lineEdit_12.text(),
                self.lineEdit_5.text(),
                self.lineEdit_4.text(),
                self.lineEdit_11.text(),
                self.lineEdit_10.text(),
                self.lineEdit_9.text(),
                self.lineEdit_3.text(),
                self.lineEdit_8.text(),
                self.lineEdit_2.text(),
                self.lineEdit.text(),
                "c.xls",####self.parametros["Planilla de Cuadratura"].get() + ".xls",
                self.lineEdit_14.text() + ".xls",                            
                self.dateEdit.date().toPyDate(),
                self.lineEdit_15.text() + ".xls",
                self.lineEdit_13.text(),
                int(self.spinBox.value()) * 60,
                self.lineEdit_16.text() + ".xls",
                self.radioButton_2.isChecked(),
                self.lineEdit_7.text(),
                self.lineEdit_6.text(),
                self.lineEdit_17.text()
            )
            self.pasos = [self.proceso.paso1, self.proceso.paso2, self.proceso.paso3, self.proceso.paso4]
            self.thread = PThread(self.pasos, 0)
            self.thread.finished.connect(self.p2Procesar)
            self.thread.start() 
        else:
            msg = QMessageBox()
            msg.setWindowTitle("Alerta")
            msg.setIcon(QMessageBox.Warning)
            msg.setText(detalle)
            msg.addButton("Cerrar", QMessageBox.YesRole)
            msg.setWindowIcon(self.windowIcon())
            ret = msg.exec_()
            

    def pError(self, e):
        msg = QMessageBox()
        msg.setWindowTitle('Ejecución Terminada')
        msg.setIcon(QMessageBox.Critical)
        msg.setText("Ha ocurrido un error:\n " + e)
        msg.addButton("Cerrar", QMessageBox.YesRole)
        msg.setWindowIcon(self.windowIcon())
        ret = msg.exec_()

        self.progressBar.setHidden(False)
        self.frame.setHidden(True)
        self.frame_2.setHidden(True)

    def p2Procesar(self):
        if self.proceso.excepcion == None:       
            del self.thread
            ####################################
            self.actualizarProgreso(25)
            #################################### 
            self.thread = PThread(self.pasos, 1)
            self.thread.finished.connect(self.p3Procesar)
            self.thread.start()    
        else:
            self.pError(self.proceso.excepcion)
            del self.thread

    def p3Procesar(self):
        if self.proceso.excepcion == None:       
            del self.thread
            ####################################
            self.actualizarProgreso(50)
            ####################################
            self.thread = PThread(self.pasos, 2)
            self.thread.finished.connect(self.p4Procesar)
            self.thread.start()    
        else:
            self.pError(self.proceso.excepcion)
            del self.thread
    
    def p4Procesar(self):
        if self.proceso.excepcion == None:       
            del self.thread
            ####################################
            self.actualizarProgreso(75)
            ####################################
            self.thread = PThread(self.pasos, 3)            
            self.thread.finished.connect(self.p5Procesar)
            self.thread.start()    
        else:
            self.pError(self.proceso.excepcion)
            del self.thread

    def p5Procesar(self):
        if self.proceso.excepcion == None:       
            del self.thread
            ####################################
            self.actualizarProgreso(100)
            ####################################            

            msg = QMessageBox()
            msg.setWindowTitle('Ejecución Terminada')
            msg.setIcon(QMessageBox.Information)
            msg.setText("Ejecución finalizada con éxito.")
            msg.addButton("Cerrar", QMessageBox.YesRole)
            msg.setWindowIcon(self.windowIcon())
            ret = msg.exec_()

            self.progressBar.setHidden(False)
            self.frame.setHidden(True)
            self.frame_2.setHidden(True)

        else:
            self.pError(self.proceso.excepcion)
            del self.thread
    
    
if __name__ == '__main__':
    app = QApplication(sys.argv)
    myWindow = Interfaz(None)
    myWindow.show()
    sys.exit(app.exec())    