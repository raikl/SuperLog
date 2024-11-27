# -*- coding: utf-8 -*-
import datetime
import xlrd
from xlutils.copy import copy
import os
import shutil
import Modelo_v14 as mod
import reporter as rep
import exportCPP as expCPP
from gurobipy import Env



class Material:
    def __init__(self, idMaterial, descripcion, sector, duracion, pesoCaja, estado, envgranel):
        self.idMaterial = idMaterial
        self.descripcion = descripcion
        self.sector = sector
        self.duracion = duracion
        self.sinPesoCaja = False
        self.sinCajasPallet = True
        self.estaEnDemanda = False
        self.tienePrecio = False
        self.cajasPallet = 0
        self.tiendas = set()
        self.envgranel = envgranel
        if pesoCaja == '(en blanco)':
            self.pesoCaja = 100.0
            self.sinPesoCaja = True
        else:
            self.pesoCaja = pesoCaja
        self.estado = estado
    def __str__(self):
        rtn = "ID: " + str(self.idMaterial) + "; "
        rtn += "DESC: " + self.descripcion + "; "
        rtn += "SECT: " + self.sector + "; "
        rtn += "DUR: " + str(self.duracion) + "; "
        rtn += "PESO: " + str(self.pesoCaja) + "; "
        rtn += "CAJxPAL: " + str(self.cajasPallet) + "; "
        rtn += "EST: " + self.estado
        return rtn
    def estaEsnTienda(self):
        rtn = False
        for e in self.tiendas:
            if isinstance(e, str) and e.startswith('T'):
                rtn = True
                break
        return rtn

class Familia:
    def __init__(self, idFamilia, estado, sector):
        self.idFamilia = idFamilia
        self.estadoSectores = {}
        self.agragar(estado, sector)
    def agragar(self, estado, sector):
        if estado not in self.estadoSectores.keys():
            self.estadoSectores[estado] = []
        self.estadoSectores[estado].append(sector)
    def __str__(self):
        rtn = "ID: " + str(self.idFamilia) + "; "
        rtn += "EST-SECT: " + str(self.estadoSectores)
        return rtn

def borrar(folder):
    for the_file in os.listdir(folder):
        file_path = os.path.join(folder, the_file)
        try:
            if os.path.isfile(file_path):
                os.unlink(file_path)
            #elif os.path.isdir(file_path): 
            #    shutil.rmtree(file_path)
        except Exception as e:
            print(e)    
               
class Modelador(mod.Modelo, rep.Reporter):
    def __formatearNodo(self, codigo):
        nodo = codigo
        if isinstance(nodo, float):
            nodo = int(nodo)
        nodo = str(nodo)
        return nodo
    
    def __generarVarNodo(self, tipo, codigo):
        nodo = self.__formatearNodo(codigo)
        if tipo == "Hub":
            rst = "Pl" + nodo
        else:
            rst = tipo[0:2] + nodo
            
        self.varNodo[rst] = nodo
        
        if tipo not in self.tipoNodoVar.keys():
            self.tipoNodoVar[tipo] = {}
        self.tipoNodoVar[tipo][nodo] = rst
        return rst

    def __obtenerVarNodo(self, nodo, tipos):
        for e in tipos:
            if nodo in self.tipoNodoVar[e].keys():
                return self.tipoNodoVar[e][nodo]

    def __obtenerTodosVarNodo(self, nodo, tipos):
        resultado = []
        for e in tipos:
            if nodo in self.tipoNodoVar[e].keys():
                resultado.append(self.tipoNodoVar[e][nodo])
        return resultado

    def __formatearNombre(self, nombre):
        nom = nombre.lower()
        return nom[0].upper() + nom[1:]


    def __init__(self, cajasPallet, costoProduccion, demandaCliente, familiaProductos, maestroMateriales, nodos, precioVenta, tarifas, stock, sets, archCuadratura, archRutas, fecha, archIndicadores, carpSalida, tiempoLimite, archDetalle, conCuadratura, compartidos, prodNoConsolida, licencia):
        strCarga = "--> cargando datos desde "
        self.prodNoConsolida = prodNoConsolida
        self.conCuadratura = conCuadratura

        self.carpetaSalidaLogistica = carpSalida + "/logistica"
        self.pathTmpLogistica = self.carpetaSalidaLogistica + "/tmp"
        self.pathLogLogistica = self.carpetaSalidaLogistica + "/log"
        self.pathSapLogistica = self.carpetaSalidaLogistica + "/SAP"

        self.carpetaSalidaAgendamiento = carpSalida + "/agendamiento"
        self.pathTmpAgendamiento = self.carpetaSalidaAgendamiento + "/tmp"
        self.pathLogAgendamiento = self.carpetaSalidaAgendamiento + "/log"
        self.pathSapAgendamiento = self.carpetaSalidaAgendamiento + "/SAP"

        borrar(carpSalida)
        if os.path.isdir(self.carpetaSalidaLogistica):
            borrar(self.carpetaSalidaLogistica)
            #print()
        else:    
            os.mkdir(self.carpetaSalidaLogistica)
        if os.path.isdir(self.pathTmpLogistica):
            borrar(self.pathTmpLogistica)
            #print()
        else:    
            os.mkdir(self.pathTmpLogistica)
        if os.path.isdir(self.pathLogLogistica):
            borrar(self.pathLogLogistica)
            #print()
        else:
            os.mkdir(self.pathLogLogistica)        
        if os.path.isdir(self.pathSapLogistica):
            borrar(self.pathSapLogistica)
            #print()
        else:
            os.mkdir(self.pathSapLogistica)        
            
        if os.path.isdir(self.carpetaSalidaAgendamiento):
            borrar(self.carpetaSalidaAgendamiento)
            #print()
        else:    
            os.mkdir(self.carpetaSalidaAgendamiento)
        if os.path.isdir(self.pathTmpAgendamiento):
            borrar(self.pathTmpAgendamiento)
            #print()
        else:    
            os.mkdir(self.pathTmpAgendamiento)
        if os.path.isdir(self.pathLogAgendamiento):
            borrar(self.pathLogAgendamiento)
            #print()
        else:
            os.mkdir(self.pathLogAgendamiento)        
        if os.path.isdir(self.pathSapAgendamiento):
            borrar(self.pathSapAgendamiento)
            #print()
        else:
            os.mkdir(self.pathSapAgendamiento)        
            
        self.archivoCuadratura = self.carpetaSalidaLogistica + "/" + archCuadratura 
        self.archivoRutas = self.carpetaSalidaLogistica + "/" + archRutas
        self.archivoIndicadores = self.carpetaSalidaLogistica + "/" + archIndicadores
        self.archivoDetalle = self.carpetaSalidaLogistica + "/" + archDetalle
        self.fecha = fecha
        self.tiempoLimite = tiempoLimite
        self.tieneSolucion = False
        self.materiales = {}
        self.destinatariosMercOperZDM = {}  
        self.minSlNodos = {}
        self.nodos = {}
        centrosNodos = {}
        sucursalesDemanda = {}
        materialCajasPallet = {}
        familias = {}
        self.IREAL = []
        self.JREAL = None
        self.JLT = None
        self.OPS = {}
        self.OETIQ = []
        self.HO = []
        self.D = []
        self.HD = []
        self.JD = []
        self.H = None
        self.SLH = []
        self.CAJASPALLET = []
        self.HPESO = []
        self.PESOCAJA = []
        self.CAP = []
        self.KG = []
        self.K = []
        self.PJH = []
        self.HPRECIOJH = []
        self.JPRECIOJH = []
        self.CH = []
        self.HCOSTOH = []        
        self.w = {}
        self.F = []
        self.congelado = []
        self.fresco = []
        self.CV2DATOS = []
        self.ICV2 = []
        self.JCV2 = []
        self.KCV2 = []
        self.JHSET = {}
        self.ddCajas = {}
        self.cons={}
        self.compartidosOrigen = []
        self.compartidosDestino = []
        self.compartidosCamiones = {}
        self.compartidosCapPal = {}
        self.maxRecorte = {}
        self.MinPallets = []
        self.MaxPallets = []
        self.sap = {"SAP1":{}, "SAP2":{}, "SAP3":{}}
        self.calzados = []
        self.calzados2 = []
        self.varNodo = {}
        self.tipoNodoVar = {}
        self.nodoTransportista = {}
        self.sectores = {}
        self.estadosMaterial = {}
        self.exportCPP = expCPP.ExportCPP()
        
        logAlertas = open(self.pathLogLogistica + "/alertas.log","w")
        ###################################
        ## Archivo: Maestro Materiales
        ###################################
        print(strCarga + maestroMateriales)
        book = xlrd.open_workbook(maestroMateriales)
        sh = book.sheet_by_name("TD")
        for row_idx in range(1, sh.nrows): 
            self.materiales[int(sh.cell_value(rowx=row_idx, colx=0))] = Material(int(sh.cell_value(rowx=row_idx, colx=0)), sh.cell_value(rowx=row_idx, colx=1), sh.cell_value(rowx=row_idx, colx=3), int(sh.cell_value(rowx=row_idx, colx=18)), sh.cell_value(rowx=row_idx, colx=19), self.__formatearNombre(sh.cell_value(rowx=row_idx, colx=20)), sh.cell_value(rowx=row_idx, colx=21))
            self.estadosMaterial[sh.cell_value(rowx=row_idx, colx=20).lower()] = sh.cell_value(rowx=row_idx, colx=17)
            self.exportCPP.agregarFila("dataMateriales", sh, row_idx, self.__formatearNombre, 20)
            
        ###################################
        ## Archivo: Nodos
        ###################################
        print(strCarga + nodos)
        book = xlrd.open_workbook(nodos)
        sh = book.sheet_by_name("Nodos")
        for row_idx in range(1, sh.nrows):
            var = self.__generarVarNodo(sh.cell_value(rowx=row_idx, colx=0), sh.cell_value(rowx=row_idx, colx=1))
            if sh.cell_value(rowx=row_idx, colx=0) == "Hub":
                self.cons[var+".."] = var+".."
            abreviacion = ""
            for p in str(sh.cell_value(rowx=row_idx, colx=4)).split():
                abreviacion += p[0]
            self.nodos[var]=(sh.cell_value(rowx=row_idx, colx=0), sh.cell_value(rowx=row_idx, colx=1), str(sh.cell_value(rowx=row_idx, colx=4)), abreviacion, var, sh.cell_value(rowx=row_idx, colx=3), sh.cell_value(rowx=row_idx, colx=2))
            self.exportCPP.agregarFila("dataNodos", sh, row_idx)

        sh = book.sheet_by_name("Destinatarios")
        for row_idx in range(1, sh.nrows):
            try:
                var = self.__obtenerVarNodo(self.__formatearNodo(sh.cell_value(rowx=row_idx, colx=0)), ["Sucursal", "Venta Directa"])
                
                if var not in centrosNodos.keys():
                    centrosNodos[var] = int(sh.cell_value(rowx=row_idx, colx=3))
                if str(sh.cell_value(rowx=row_idx, colx=0)).startswith("T") and sh.cell_value(rowx=row_idx, colx=2).startswith("CCS"):# and "_" not in str(sh.cell_value(rowx=row_idx, colx=0)):
                    self.minSlNodos[var+"S"] = float(str(sh.cell_value(rowx=row_idx, colx=4)).replace("*SL", "").replace(",", "."))
                else:
                    self.minSlNodos[var] = float(str(sh.cell_value(rowx=row_idx, colx=4)).replace("*SL", "").replace(",", "."))
                if var not in self.destinatariosMercOperZDM.keys():
                    self.destinatariosMercOperZDM[var] = []
                self.destinatariosMercOperZDM[var].append(sh.cell_value(rowx=row_idx, colx=2))
            except:
                print("Destinatario no cargado:", self.__formatearNodo(sh.cell_value(rowx=row_idx, colx=0)))
        
        sh = book.sheet_by_name("Transportes")
        for row_idx in range(1, sh.nrows):             
            if len(sh.cell_value(rowx=row_idx, colx=0)) > 0:
                self.exportCPP.agregarFila("dataTransportes", sh, row_idx)
                self.CAP.append(int(sh.cell_value(rowx=row_idx, colx=2)))
                self.KG.append(int(sh.cell_value(rowx=row_idx, colx=1)))
                self.K.append(sh.cell_value(rowx=row_idx, colx=0).replace(" ", ""))
                self.MinPallets.append(int(sh.cell_value(rowx=row_idx, colx=3)))
                self.MaxPallets.append(int(sh.cell_value(rowx=row_idx, colx=4)))

        for e in self.sap.keys():
            sh = book.sheet_by_name(e)
            c1 = sh.cell_value(rowx=0, colx=1).lower()
            c2 = sh.cell_value(rowx=0, colx=2).lower()
            self.sap[e][c1] = {}
            self.sap[e][c2] = {}
            for row_idx in range(1, sh.nrows): 
                if len(sh.cell_value(rowx=row_idx, colx=0)) > 0:
                    self.sap[e][c1][sh.cell_value(rowx=row_idx, colx=0)] = sh.cell_value(rowx=row_idx, colx=1)
                    self.sap[e][c2][sh.cell_value(rowx=row_idx, colx=0)] = sh.cell_value(rowx=row_idx, colx=2)

        sh = book.sheet_by_name("Sectores")
        for row_idx in range(1, sh.nrows): 
            if len(sh.cell_value(rowx=row_idx, colx=0)) > 0:
                self.sectores[sh.cell_value(rowx=row_idx, colx=0)] = sh.cell_value(rowx=row_idx, colx=1)
        

        ###################################
        ## Archivo: Productos que No Consolida
        ###################################
        print(strCarga + prodNoConsolida)
        book = xlrd.open_workbook(prodNoConsolida)
        sh = book.sheet_by_name("Calzado")
        for row_idx in range(1, sh.nrows): 
            var = self.__obtenerVarNodo(self.__formatearNodo(sh.cell_value(rowx=row_idx, colx=2)), ["Sucursal", "Venta Directa"])
            self.calzados.append((int(sh.cell_value(rowx=row_idx, colx=0)), var))
        sh = book.sheet_by_name("Calzado2")
        for row_idx in range(1, sh.nrows): 
            self.calzados2.append(int(sh.cell_value(rowx=row_idx, colx=0)))


        ###################################
        ## Archivo: Compartidos
        ###################################
        print(strCarga + compartidos)
        book = xlrd.open_workbook(compartidos)
        sh = book.sheet_by_name("Compartidos")
        for row_idx in range(1, sh.nrows):
            varO = self.__obtenerVarNodo(self.__formatearNodo(sh.cell_value(rowx=row_idx, colx=0)), ["Sucursal"])
            varD = self.__obtenerVarNodo(self.__formatearNodo(sh.cell_value(rowx=row_idx, colx=2)), ["Sucursal"])
            self.compartidosOrigen.append(varO)
            self.compartidosDestino.append(varD)
        sh = book.sheet_by_name("Camiones")
        for row_idx in range(1, sh.nrows): 
            nroTransportes = -1
            capCamion = -1
            try:
                nroTransportes = float(sh.cell_value(rowx=row_idx, colx=2))
            except Exception as e:
                pass
            try:
                capCamion = float(sh.cell_value(rowx=row_idx, colx=3))            
            except Exception as e:
                pass
            
            varO = self.__obtenerVarNodo(self.__formatearNodo(sh.cell_value(rowx=row_idx, colx=0)), ["Sucursal"])
            self.compartidosCamiones[varO] = nroTransportes
            if nroTransportes > 0:
                if capCamion > 0 and capCamion < 27:
                    self.compartidosCapPal[varO] = capCamion
                else:
                    self.compartidosCapPal[varO] = 24

        
        ###################################
        ## Archivo: Demanda Clientes
        ###################################        
        print(strCarga + demandaCliente)
        book = xlrd.open_workbook(demandaCliente)
        sh = book.sheet_by_name("Demanda")
        ddaDuplicada = {}
        for row_idx in range(1, sh.nrows):
            var = self.__obtenerVarNodo(self.__formatearNodo(sh.cell_value(rowx=row_idx, colx=4)), ["Sucursal", "Venta Directa"])
            self.exportCPP.agregarFila("dataDemanda", sh, row_idx)

            sucursalesDemanda[var] = centrosNodos[var]                    
            if isinstance(sh.cell_value(rowx=row_idx, colx=6), str):
                self.D.append(0)
            else:
                self.D.append(sh.cell_value(rowx=row_idx, colx=6))
            if sh.cell_value(rowx=row_idx, colx=5).startswith("CCS"): # and not sh.cell_value(rowx=row_idx, colx=5).endswith("_2"):
                self.JD.append(var+"S")
                self.ddCajas[str(var)+"S", int(sh.cell_value(rowx=row_idx, colx=2))] = sh.cell_value(rowx=row_idx, colx=6)
            else:
                self.JD.append(var)
                self.ddCajas[str(var), int(sh.cell_value(rowx=row_idx, colx=2))] = sh.cell_value(rowx=row_idx, colx=6)
            self.HD.append(int(sh.cell_value(rowx=row_idx, colx=2)))
            self.materiales[int(sh.cell_value(rowx=row_idx, colx=2))].estaEnDemanda = True
            self.materiales[int(sh.cell_value(rowx=row_idx, colx=2))].tiendas.add(var)
            if (sh.cell_value(rowx=row_idx, colx=2), sh.cell_value(rowx=row_idx, colx=5)) in ddaDuplicada.keys():
                ddaDuplicada[(sh.cell_value(rowx=row_idx, colx=2), sh.cell_value(rowx=row_idx, colx=5))] += 1
            else:
                ddaDuplicada[(sh.cell_value(rowx=row_idx, colx=2), sh.cell_value(rowx=row_idx, colx=5))] = 1
        self.JREAL = list(sucursalesDemanda.keys())
        self.JLT = list(sucursalesDemanda.values())
        self.H = list(set(self.HD))  
        for e in self.H:
            self.SLH.append(self.materiales[e].duracion)

        
        ######################################################################
        ## VALIDACION: Demanda duplicada
        ######################################################################
        ddadup = []
        for e in ddaDuplicada:
            if ddaDuplicada[e] > 1:
                ddadup.append(e)
        if len(ddadup) > 0:
            logAlertas.write("  Productos con demanda duplicada - validación: " + str(ddadup) + "\n")
            logAlertas.close() 
            raise ValueError("Error de validación. El detalle se encuentra en el log de ejecución.")

        
        ###################################
        ## Archivo: Stock
        ###################################
        print(strCarga + stock)
        logAlertas.write(strCarga + stock + "\n") 
        sinDda = set()        
        book = xlrd.open_workbook(stock)
        sh = book.sheet_by_name("Stock")
        self.exportCPP.agregarFila("dataStock", sh, 0)
        for col_idx in range(5, sh.ncols): 
            val = self.__obtenerVarNodo(self.__formatearNodo(sh.cell_value(rowx=0, colx=col_idx)), ["Hub", "Planta"])

            self.IREAL.append(val)
            for row_idx in range(1, sh.nrows):
                if val not in self.OPS.keys():
                    self.OPS[val] = []
                # Validacion de Productos en Demanda
                if self.materiales[int(sh.cell_value(rowx=row_idx, colx=2))].estaEnDemanda:
                    self.OPS[val].append(sh.cell_value(rowx=row_idx, colx=col_idx))
        for row_idx in range(1, sh.nrows):
            dtime = sh.cell_value(rowx=row_idx, colx=4)
            
            # Validacion de Productos en Demanda
            if self.materiales[int(sh.cell_value(rowx=row_idx, colx=2))].estaEnDemanda:
                if isinstance(dtime, float):
                    self.OETIQ.append(datetime.datetime.fromtimestamp(dtime*3600*24) - dateutil.relativedelta.relativedelta(years=70) - dateutil.relativedelta.relativedelta(days=1) - dateutil.relativedelta.relativedelta(hours=20))
                else:
                    self.OETIQ.append(dtime)
                self.HO.append(int(sh.cell_value(rowx=row_idx, colx=2)))
            else:
                sinDda.add(str(int(sh.cell_value(rowx=row_idx, colx=2))))
        if len(sinDda) > 0:
            logAlertas.write("  Productos que no tienen Demanda: " + str(sinDda) + "\n")

        
        ###################################
        ## Archivo: Cajas-Pallet
        ################################### 
        print(strCarga + cajasPallet)
        logAlertas.write(strCarga + cajasPallet + "\n")
        sinDda = set()        
        book = xlrd.open_workbook(cajasPallet)
        sh = book.sheet_by_name("Consolidado")
        for row_idx in range(1, sh.nrows): 
            self.exportCPP.agregarFila("dataCajasPallet", sh, row_idx)
            # Validacion de Productos en Demanda
            if self.materiales[int(sh.cell_value(rowx=row_idx, colx=0))].estaEnDemanda:
                materialCajasPallet[int(sh.cell_value(rowx=row_idx, colx=0))] = int(sh.cell_value(rowx=row_idx, colx=2))
            else:
                sinDda.add(str(int(sh.cell_value(rowx=row_idx, colx=0))))
        if len(sinDda) > 0:
            logAlertas.write("  Productos que no tienen Demanda: " + str(sinDda) + "\n")
        for e in self.materiales:
            # Validacion de Productos en Demanda
            if self.materiales[e].estaEnDemanda:
                if e in materialCajasPallet.keys():
                    self.CAJASPALLET.append(materialCajasPallet[e])
                    self.materiales[e].cajasPallet = materialCajasPallet[e]
                    self.materiales[e].sinCajasPallet = False
                else:
                    self.CAJASPALLET.append(50)
                self.HPESO.append(e)
                self.PESOCAJA.append(self.materiales[e].pesoCaja)


        ###################################
        ## Archivo: Precio Venta
        ###################################
        print(strCarga + precioVenta)
        logAlertas.write(strCarga + precioVenta + "\n")
        sinDda = set() 
        precioventajh = []        
        book = xlrd.open_workbook(precioVenta)
        sh = book.sheet_by_name("Precios")
        for row_idx in range(1, sh.nrows): 
            var = self.__obtenerVarNodo(self.__formatearNodo(sh.cell_value(rowx=row_idx, colx=3)), ["Sucursal", "Venta Directa"])
            self.exportCPP.agregarFila("dataPreciosVenta", sh, row_idx)

            if var in self.materiales[int(sh.cell_value(rowx=row_idx, colx=1))].tiendas:                        
                if sh.cell_value(rowx=row_idx, colx=4).startswith("CCS"):
                    self.PJH.append(float(format(sh.cell_value(rowx=row_idx, colx=5), '.2f')))
                    self.HPRECIOJH.append(int(sh.cell_value(rowx=row_idx, colx=1)))
                    self.JPRECIOJH.append(var+"S")
                    precioventajh.append((var+"S", int(sh.cell_value(rowx=row_idx, colx=1))))
                else:
                    self.PJH.append(float(format(sh.cell_value(rowx=row_idx, colx=5), '.2f')))
                    self.HPRECIOJH.append(int(sh.cell_value(rowx=row_idx, colx=1)))
                    self.JPRECIOJH.append(var)
                    precioventajh.append((var, int(sh.cell_value(rowx=row_idx, colx=1))))
                
            else:
                sinDda.add("["+str(var)+";"+str(int(sh.cell_value(rowx=row_idx, colx=1)))+"]")
            self.materiales[int(sh.cell_value(rowx=row_idx, colx=1))].tienePrecio = True
        if len(sinDda) > 0:
            logAlertas.write("  Productos que no tienen Demanda: " + str(sinDda) + "\n")


        ###################################
        ## Archivo: Costo Produccion
        ###################################
        print(strCarga + costoProduccion)
        logAlertas.write(strCarga + costoProduccion + "\n")
        sinDda = set()
        noExisteMM = set()
        book = xlrd.open_workbook(costoProduccion)
        sh = book.sheet_by_name("Hoja1")
        for row_idx in range(1, sh.nrows): 
            self.exportCPP.agregarFila("dataCostoProduccion", sh, row_idx)
            # Validacion de Productos en Demanda
            if int(sh.cell_value(rowx=row_idx, colx=1)) in self.materiales.keys():
                if self.materiales[int(sh.cell_value(rowx=row_idx, colx=1))].estaEnDemanda:        
                    var = sh.cell_value(rowx=row_idx, colx=3)
                    if isinstance(var, str):
                        var = 0
                    self.CH.append(var)
                    self.HCOSTOH.append(int(sh.cell_value(rowx=row_idx, colx=1)))
                else:
                    sinDda.add(str(int(sh.cell_value(rowx=row_idx, colx=1))))
            else:
                noExisteMM.add(str(int(sh.cell_value(rowx=row_idx, colx=1))))
        if len(sinDda) > 0:
            logAlertas.write("  Productos que no tienen Demanda: " + str(sinDda) + "\n")
        if len(noExisteMM) > 0:
            logAlertas.write("  Productos que no existen en Maestro Materiales: " + str(noExisteMM) + "\n")


        ###################################
        ## Archivo: Familia de Productos
        ###################################
        print(strCarga + familiaProductos)
        book = xlrd.open_workbook(familiaProductos)
        sh = book.sheet_by_name("Familias")
        for row_idx in range(1, sh.nrows): 
            self.exportCPP.agregarFila("dataFamilias", sh, row_idx)
            if int(sh.cell_value(rowx=row_idx, colx=0)) not in familias.keys():
                familias[int(sh.cell_value(rowx=row_idx, colx=0))] = Familia(int(sh.cell_value(rowx=row_idx, colx=0)), sh.cell_value(rowx=row_idx, colx=1), sh.cell_value(rowx=row_idx, colx=2))
                self.w[int(sh.cell_value(rowx=row_idx, colx=0))] = []
                if sh.cell_value(rowx=row_idx, colx=3) == "si":
                    self.congelado.append(int(sh.cell_value(rowx=row_idx, colx=0)))
                else:
                    self.fresco.append(int(sh.cell_value(rowx=row_idx, colx=0)))
            else:
                familias[int(sh.cell_value(rowx=row_idx, colx=0))].agragar(sh.cell_value(rowx=row_idx, colx=1), sh.cell_value(rowx=row_idx, colx=2))
        self.F = list(familias.keys())
        for e in self.materiales:
            for f in familias.keys():
                if self.materiales[e].estado in familias[f].estadoSectores.keys() and self.materiales[e].sector in familias[f].estadoSectores[self.materiales[e].estado] and self.materiales[e].estaEnDemanda: # Validacion de Productos en Demanda
                    self.w[f].append(self.materiales[e].idMaterial)


        ###################################
        ## Archivo: Tarifas
        ###################################
        print(strCarga + tarifas)
        book = xlrd.open_workbook(tarifas)
        sh = book.sheet_by_name("Tarifas")
        for row_idx in range(1, sh.nrows): 
            varI = self.__obtenerTodosVarNodo(self.__formatearNodo(sh.cell_value(rowx=row_idx, colx=0)), ["Hub", "Planta", "Sucursal", "Venta Directa"])
            varJ = self.__obtenerTodosVarNodo(self.__formatearNodo(sh.cell_value(rowx=row_idx, colx=2)), ["Hub", "Planta", "Sucursal", "Venta Directa"])
            self.exportCPP.agregarFila("dataCV", sh, row_idx)
            for vi in varI:
                for vj in varJ:
                    if (self.nodos[vi][0] in ["Hub", "Planta"] and self.nodos[vj][0] in ["Hub", "Planta", "Sucursal", "Venta Directa"]) or (self.nodos[vi][0] == "Sucursal" and self.nodos[vj][0] == "Sucursal") or (self.nodos[vi][0] == "Venta Directa" and self.nodos[vj][0] == "Venta Directa"):
                        self.ICV2.append(vi)
                        self.JCV2.append(vj)
                        self.KCV2.append(sh.cell_value(rowx=row_idx, colx=4).replace(" ", ""))
                        self.CV2DATOS.append(int(sh.cell_value(rowx=row_idx, colx=5)))          

        sh = book.sheet_by_name("Transportistas")
        for row_idx in range(1, sh.nrows): 
            var = self.__obtenerVarNodo(self.__formatearNodo(sh.cell_value(rowx=row_idx, colx=0)), ["Hub", "Planta", "Sucursal", "Venta Directa"])
            self.nodoTransportista[var] = int(sh.cell_value(rowx=row_idx, colx=1))
                    
        sh = book.sheet_by_name("Tiempos de Viaje")
        for row_idx in range(1, sh.nrows): 
            if len(str(sh.cell_value(rowx=row_idx, colx=0))) > 0:
                self.exportCPP.agregarFila("dataTViajes", sh, row_idx)

        
        ###################################
        ## Archivo: Set
        ###################################
        print(strCarga + sets)
        logAlertas.write(strCarga + sets + "\n")
        sinDda = set()
        sinPVta = set()
        setDuplicados = {}
        book = xlrd.open_workbook(sets)
        sh = book.sheet_by_name("Set")
        for row_idx in range(4, sh.nrows): 
            varJ = self.__obtenerVarNodo(self.__formatearNodo(sh.cell_value(rowx=row_idx, colx=0)), ["Sucursal", "Venta Directa"])
                
            if sh.cell_value(rowx=row_idx, colx=8) == "TRUE":
                self.maxRecorte[varJ, int(sh.cell_value(rowx=row_idx, colx=1))] = float(sh.cell_value(rowx=row_idx, colx=9))

            if sh.cell_value(rowx=row_idx, colx=6) == "TRUE":
                if varJ in self.materiales[int(sh.cell_value(rowx=row_idx, colx=1))].tiendas:
                    if (varJ, int(sh.cell_value(rowx=row_idx, colx=1))) in self.ddCajas:
                        if (varJ, int(sh.cell_value(rowx=row_idx, colx=1))) in precioventajh:
                            self.JHSET[varJ, int(sh.cell_value(rowx=row_idx, colx=1))] = float(sh.cell_value(rowx=row_idx, colx=7))
                        else:
                            sinPVta.add("["+str(varJ)+";"+str(int(sh.cell_value(rowx=row_idx, colx=1)))+"]")                    
                    elif (varJ+"S",int(sh.cell_value(rowx=row_idx, colx=1))) in self.ddCajas:
                        if (varJ+"S", int(sh.cell_value(rowx=row_idx, colx=1))) in precioventajh:
                            self.JHSET[varJ+"S", int(sh.cell_value(rowx=row_idx, colx=1))] = float(sh.cell_value(rowx=row_idx, colx=7))
                        else:
                            sinPVta.add("["+str(varJ)+"S;"+str(int(sh.cell_value(rowx=row_idx, colx=1)))+"]")                    
                else:
                    sinDda.add("["+str(varJ)+";"+str(int(sh.cell_value(rowx=row_idx, colx=1)))+"]")
                    
            if (sh.cell_value(rowx=row_idx, colx=0), sh.cell_value(rowx=row_idx, colx=1)) in setDuplicados.keys():
                setDuplicados[(sh.cell_value(rowx=row_idx, colx=0), sh.cell_value(rowx=row_idx, colx=1))] += 1
            else:
                setDuplicados[(sh.cell_value(rowx=row_idx, colx=0), sh.cell_value(rowx=row_idx, colx=1))] = 1

        if len(sinDda) > 0:
            logAlertas.write("  Productos en SET que no tienen demanda - validación: " + str(sinDda) + "\n")
        if len(sinPVta) > 0:
            logAlertas.write("  Productos en SET que no tienen precio de venta - validación: " + str(sinPVta) + "\n")

                
        ######################################################################
        ## VALIDACION: SET duplicados
        ######################################################################
        setdup = []
        for e in setDuplicados:
            if setDuplicados[e] > 1:
                setdup.append(e)
        if len(setdup) > 0:
            logAlertas.write("  Productos duplicada en SET - validación: " + str(setdup) + "\n")
            logAlertas.close() 
            raise ValueError("Error de validación. El detalle se encuentra en el log de ejecución.")

        ######################################################################
        ## VALIDACION: Todos los productos en demanda deben tener precio
        ######################################################################
        errorDdaPrecio = set()
        for e in self.materiales:
            if self.materiales[e].estaEnDemanda and not self.materiales[e].tienePrecio and self.materiales[e].estaEsnTienda():
                errorDdaPrecio.add(e)
        if len(errorDdaPrecio) > 0:
            logAlertas.write("  Productos con demanda y sin precio - validación: " + str(errorDdaPrecio) + "\n")
            logAlertas.close() 
            raise ValueError("Error de validación. El detalle se encuentra en el log de ejecución.")

        ######################################################################
        ## VALIDACION: Todos los i y j esten en Tarifas
        ######################################################################
        erroresI = ""
        erroresJPv = ""
        erroresJDda = ""
        setICV = set(self.ICV2)
        setJCV = set(self.JCV2)
        setJPRECIOJH = set(self.JPRECIOJH)
        for i in self.IREAL:
            if i not in setICV:
                if len(erroresI) > 0:
                    erroresI += ", "
                erroresI += str(i)
        for j in self.JREAL:
            if j not in setJCV:
                if len(erroresJDda) > 0:
                    erroresJDda += ", "
                erroresJDda += str(j)
        for j in self.JREAL: #Sólo en sucursales
            vr = j
            if isinstance(vr, int) and vr == 60472:
                    vr = "P" + str(vr) 
            if j not in setJPRECIOJH and self.nodos[vr][0] == "Sucursal":
                if len(erroresJPv) > 0:
                    erroresJPv += ", "
                erroresJPv += str(j)
        
        if len(erroresI) > 0 or len(erroresJPv) > 0 or len(erroresJDda) > 0:
            print("Error en los nodos I del archivo Stock:", erroresI)
            logAlertas.write("-----------------------------------------\n")
            logAlertas.write("\tError en los nodos I del archivo Stock:" + erroresI + "\n")            
            print("Error en los nodos J del archivo Demanda Cliente:", erroresJDda)
            logAlertas.write("\tError en los nodos J del archivo Demanda Cliente:" + erroresJDda + "\n")            
            print("Error en los nodos J del archivo Precio Venta:", erroresJPv)
            logAlertas.write("\tError en los nodos J del archivo Precio Venta:" + erroresJPv + "\n")      
            logAlertas.close()            
            raise ValueError("Error de validación. El detalle se encuentra en el log de ejecución.")

        ######################################################################
        ## VALIDACION: Todos los materiales con demanda deben tener definidas
        ## cajaspallet, pesocaja y slh. Además, cajaspallet x pesocaja <= 5000
        ## ---> slh se evalúa al contruir el diccionario self.materiales
        ######################################################################
        erroresCajasPallet = ""
        erroresPesoCaja = ""
        erroresLimitePeso = ""
        for e in self.H:
            if self.materiales[e].sinCajasPallet:
                if len(erroresCajasPallet) > 0:
                    erroresCajasPallet += ", "
                erroresCajasPallet += str(e)
            if self.materiales[e].sinPesoCaja:
                if len(erroresPesoCaja) > 0:
                    erroresPesoCaja += ", "
                erroresPesoCaja += str(e)
            if not self.materiales[e].sinCajasPallet and not self.materiales[e].sinPesoCaja and self.materiales[e].pesoCaja * self.materiales[e].cajasPallet > 5000:            
                if len(erroresLimitePeso) > 0:
                    erroresLimitePeso += ", "
                erroresLimitePeso += str(e)
        if len(erroresCajasPallet) > 0 or len(erroresPesoCaja) > 0 or len(erroresLimitePeso) > 0:
            print("Materiales con error en Cajas-Pallet:", erroresCajasPallet)
            logAlertas.write("-----------------------------------------\n")
            logAlertas.write("\tMateriales con error en Cajas-Pallet:" + erroresCajasPallet + "\n")
            print("Materiales con error en Peso x Caja:", erroresPesoCaja)
            logAlertas.write("\tMateriales con error en Peso x Caja:" + erroresPesoCaja + "\n")
            print("Materiales con error en Límite de Peso:", erroresLimitePeso)
            logAlertas.write("\tMateriales con error en Límite de Peso: " + erroresLimitePeso + "\n")
            logAlertas.close()
            raise ValueError("Error de validación. El detalle se encuentra en el log de ejecución.")

        ###################################
        ## Archivo: Licencia
        ###################################
        self.env = None
        if len(licencia) > 0:
            print(strCarga + licencia)
            env = Env(empty=True)
            arch = open(licencia, "r") 
            for l in arch.readlines():
                spt = l.rstrip('\n').split("=")
                if len(spt) != 2:
                    raise ValueError("Error en el archivo de licencia.")
                #print(spt)
                env.setParam(spt[0], spt[1])
            env.start()
            self.env = env

        logAlertas.close()
        
        self.exportCPP.grabarAlmacen(self.pathTmpLogistica + "/exportCPP.txt")
        
                
    def ejecutar(self):
        print("-Ejecutando Modelo-")
        self.modelar()
        
