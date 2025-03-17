#ifndef CARGA_H
#define CARGA_H

#include <fstream>
#include <iostream>
#include <set>
#include <map>
#include <vector>
#include <string>
#include <regex>
#include <algorithm>
#include <exception>
#include "importPython.h"
#include "Graph_structDFS.hpp"

using namespace std;

struct DefinicionFamilia {
	int id;
	set<string> sectores;
	set<string> estados;
	int pos;
};


bool endsWith(const string& str, const string& suffix) {
	return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

bool startsWith(const string& str, const string& prefix) {
	return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

void replaceAll(string &s, const string &search, const string &replace) {
	for (size_t pos = 0;; pos += replace.length()) {
		// Locate the substring to replace
		pos = s.find(search, pos);
		if (pos == string::npos) break;
		// Replace by erasing and inserting
		s.erase(pos, search.length());
		s.insert(pos, replace);
	}
}

vector<string> fromString(string str) {
	vector<string> elements;

	try {
		regex r("[^\\]\\[,\"]+");

		auto it = std::sregex_iterator(str.begin(), str.end(), r);
		auto end = std::sregex_iterator();
	
		for (; it != end; ++it) {
			auto match = *it;
			auto element = match.str();
			auto it2 = it;
			++it2;
			if (it2 != end) {
				replaceAll(element, ".", "");
			}
			elements.push_back(element);
			//cout << element << endl;
		}
	}
	catch (std::exception const& e) {
		std::cout << e.what() << "\n";
	}
	return elements;
}



void scan_links(ifstream& inputa, Graph& G, vector<Edge>& Edges, vector <Node>& Nodes, vector<Producto>& Prod)
{
	vector<string> row;
	string line, topic;
	while (!inputa.eof())
	{
		row.clear();
		getline(inputa, line, '\n');

		if (!line.empty())
		{
			//istringstream s(line);
			//while (getline(s, topic, ','))
			//	row.push_back(topic);

			//cout << line << '\n';
			//vector<string> partes = split(row[0], "_", false);
			vector<string> finalp = fromString(line);
			/*for (auto i : partes) {
				string str = i;
				replaceAll(str, ".", "");
				finalp.push_back(str);
			}*/
			//finalp.push_back(row[1]);

			string NOF, NDF, NDFF;
			bool superm;
            bool nodeAtr;
            int nodeDescarga;   // ítem que indica cual es el nodo de descarga en caso de xs y zs, vale 1 si es primer sucursal, 2 si es la segunda, -1 si es de otro tipo
			G.Find_nodes(finalp, &NOF, &NDF, &NDFF, &superm, &nodeAtr, &nodeDescarga);
            
            if(!nodeAtr) // caso de leer atributo para arco
            {
                int NOi = G.get_Node_id(NOF, Nodes);
                int NDi = G.get_Node_id(NDF, Nodes);
                int ND2i = G.get_Node_id(NDFF, Nodes);
                        
                if((NOi != -1)&&(NDi != -1))    //tramo único o primer tramo
                {
                    bool pr = false;
                    if(ND2i != -1)  // nodo ND2i es un sucesor
                        pr = false;
                        
                    int pp = G.checkEdge(NOF, NDF, Edges, Nodes);
                    if (pp == -1)
                        G.addEdge(NOi, NDi, ND2i, pr, Edges, finalp, superm, nodeDescarga, Prod);
                    else
                        G.addAtributo(pp, ND2i, pr, Edges, finalp, superm, nodeDescarga, Prod);
                }
                if((NDi != -1)&&(ND2i != -1))   //segundo tramo
                {
                    bool pr = false;
                    if(NOi != -1)  // nodo ND2i es un predecesor
                        pr = true;
                    
                    int pp = G.checkEdge(NDF, NDFF, Edges, Nodes);
                    if (pp == -1)
                        G.addEdge(NDi, ND2i, NOi, pr, Edges, finalp, superm, nodeDescarga, Prod);
                    else
                        G.addAtributo(pp, NOi, pr, Edges, finalp, superm, nodeDescarga, Prod);
                }
            }   // end caso de leer atributo para arco
            else  // caso de leer atributo para nodo oferta o demanda
            {
                int NOi = G.get_Node_id(NOF, Nodes);
                if(finalp[0]=="o1" && stod(finalp[4]) > UmbralMinimoAmount)
                {
                    Nodes[NOi].o1.push_back(Prod_parts(finalp[1],stod(finalp[4]),superm));
                    Nodes[NOi].o1.back().frescura = stoi(finalp[2]);
                }
                if(finalp[0]=="o2" && stod(finalp[4]) > UmbralMinimoAmount)
                {
                    Nodes[NOi].o2.push_back(Prod_parts(finalp[1],stod(finalp[4]),superm));
                    Nodes[NOi].o2.back().frescura = stoi(finalp[2]);
                }
                if(finalp[0]=="u" && stod(finalp[4]) > UmbralMinimoAmount)
                {
                    Nodes[NOi].u.push_back(Prod_parts(finalp[1],stod(finalp[4]),superm));
                    Nodes[NOi].u.back().frescura = stoi(finalp[2]);
                }
                
            } // end caso de leer atributo para nodo oferta o demanda
            
		}   // end if recorriendo line

	}   // end while inputa
    
    //actualizo atributos de los arcos, así se mantiene relación entre arcos
    G.Actualiza_arcos_compuestos(Edges, Nodes);

}   // end scan_links

void scan_linksCUAD(ifstream& inputa, ifstream& inputCUAD, Graph& G, vector<Edge>& Edges, vector <Node>& Nodes, vector<Producto>& Prod)
{
    vector<string> row;
    string line, topic;
    
    while (!inputa.eof())       //lee output modelo sin cuadratura
    {
        row.clear();
        getline(inputa, line, '\n');

        if (!line.empty())
        {
            //istringstream s(line);
            //while (getline(s, topic, ','))
            //    row.push_back(topic);
            //vector<string> partes = split(row[0], "_", false);
            vector<string> finalp = fromString(line);
            /*for (auto i : partes) {
                string str = i;
                replaceAll(str, ".", "");
                finalp.push_back(str);
            }*/
            //finalp.push_back(row[1]);
            
            string NOF, NDF, NDFF;
            bool superm;
            bool nodeAtr;
            int nodeDescarga;   // ítem que indica cual es el nodo de descarga en caso de xs y zs, vale 1 si es primer sucursal, 2 si es la segunda, -1 si es de otro tipo
            G.Find_nodes(finalp, &NOF, &NDF, &NDFF, &superm, &nodeAtr, &nodeDescarga);
            
            if(!nodeAtr) // caso de leer atributo para arco
            {
                if((finalp[0]=="x")||(finalp[0]=="x2")||(finalp[0]=="xr")||(finalp[0]=="xs")||(finalp[0]=="y")||(finalp[0]=="y2")||(finalp[0]=="ys"))      //único caso donde tengo que leer estos atributos del archivo sin cuadratura, arcos.
                {
                    int NOi = G.get_Node_id(NOF, Nodes);
                    int NDi = G.get_Node_id(NDF, Nodes);
                    int ND2i = G.get_Node_id(NDFF, Nodes);
                                    
                    if((NOi != -1)&&(NDi != -1))    //tramo único o primer tramo
                    {
                        bool pr = false;
                        if(ND2i != -1)  // nodo ND2i es un sucesor
                            pr = false;
                                    
                        int pp = G.checkEdge(NOF, NDF, Edges, Nodes);
                        if (pp == -1)
                            G.addEdge(NOi, NDi, ND2i, pr, Edges, finalp, superm, nodeDescarga, Prod);
                        else
                            G.addAtributo(pp, ND2i, pr, Edges, finalp, superm, nodeDescarga, Prod);
                    }
                    if((NDi != -1)&&(ND2i != -1))   //segundo tramo
                    {
                        bool pr = false;
                        if(NOi != -1)  // nodo ND2i es un predecesor
                            pr = true;
                                
                        int pp = G.checkEdge(NDF, NDFF, Edges, Nodes);
                        if (pp == -1)
                            G.addEdge(NDi, ND2i, NOi, pr, Edges, finalp, superm, nodeDescarga, Prod);
                        else
                            G.addAtributo(pp, NOi, pr, Edges, finalp, superm, nodeDescarga, Prod);
                    }
                }       // end if lee solo atributos de flujo para arco
        
            }   // end caso de leer atributo para arco
            
            else  // caso de leer atributo para nodo oferta o demanda
            {
                int NOi = G.get_Node_id(NOF, Nodes);
                if((finalp[0]=="o1") && (stod(finalp[4]) > UmbralMinimoAmount))
                {
                    Nodes[NOi].o1.push_back(Prod_parts(finalp[1],stod(finalp[4]),superm));
                    Nodes[NOi].o1.back().frescura = stoi(finalp[2]);
                }
                if((finalp[0]=="o2") && (stod(finalp[4]) > UmbralMinimoAmount))
                {
                    Nodes[NOi].o2.push_back(Prod_parts(finalp[1],stod(finalp[4]),superm));
                    Nodes[NOi].o2.back().frescura = stoi(finalp[2]);
                }
                if((finalp[0]=="u") && (stod(finalp[4]) > UmbralMinimoAmount))
                {
                    Nodes[NOi].u.push_back(Prod_parts(finalp[1],stod(finalp[4]),superm));
                    Nodes[NOi].u.back().frescura = stoi(finalp[2]);
                }
                
            } // end caso de leer atributo para nodo oferta o demanda
            
        }   // end if recorriendo line

    }   // end while inputa
    
    //leyendo archivo con cuadratura
    while (!inputCUAD.eof())       //lee output modelo con cuadratura
    {
        row.clear();
        getline(inputCUAD, line, '\n');

        if (!line.empty())
        {
            /*istringstream s(line);
            while (getline(s, topic, ','))
                row.push_back(topic);

            vector<string> partes = split(row[0], "_", false);*/
			vector<string> finalp = fromString(line);
            /*for (auto i : partes) {
                string str = i;
                replaceAll(str, ".", "");
                finalp.push_back(str);
            }
            finalp.push_back(row[1]);*/

            string NOF, NDF, NDFF;
            bool superm;
            bool nodeAtr;
            int nodeDescarga;   // ítem que indica cual es el nodo de descarga en caso de xs y zs, vale 1 si es primer sucursal, 2 si es la segunda, -1 si es de otro tipo
            G.Find_nodes(finalp, &NOF, &NDF, &NDFF, &superm, &nodeAtr, &nodeDescarga);
            
            if(!nodeAtr) // caso de leer atributo para arco
            {
                int NOi = G.get_Node_id(NOF, Nodes);
                int NDi = G.get_Node_id(NDF, Nodes);
                int ND2i = G.get_Node_id(NDFF, Nodes);
                           
                if((NOi != -1)&&(NDi != -1))    //tramo único o primer tramo
                {
                    bool pr = false;
                    if(ND2i != -1)  // nodo ND2i es un sucesor
                        pr = false;
                           
                    int pp = G.checkEdge(NOF, NDF, Edges, Nodes);
                    if (pp == -1)
                        G.addEdge(NOi, NDi, ND2i, pr, Edges, finalp, superm, nodeDescarga, Prod);
                    else
                        G.addAtributo(pp, ND2i, pr, Edges, finalp, superm, nodeDescarga, Prod);
                }
                if((NDi != -1)&&(ND2i != -1))   //segundo tramo
                {
                    bool pr = false;
                    if(NOi != -1)  // nodo ND2i es un predecesor
                        pr = true;
                       
                    int pp = G.checkEdge(NDF, NDFF, Edges, Nodes);
                    if (pp == -1)
                        G.addEdge(NDi, ND2i, NOi, pr, Edges, finalp, superm, nodeDescarga, Prod);
                    else
                        G.addAtributo(pp, NOi, pr, Edges, finalp, superm, nodeDescarga, Prod);
                   }
               }   // end caso de leer atributo para arco
               else  // caso de leer atributo para nodo oferta o demanda
               {
                   int NOi = G.get_Node_id(NOF, Nodes);
                   if((finalp[0]=="qo1") && (stod(finalp[4]) > UmbralMinimoAmount))
                   {
                       Nodes[NOi].o1.push_back(Prod_parts(finalp[1],stod(finalp[4]),superm));
                       Nodes[NOi].o1.back().frescura = stoi(finalp[2]);
                   }
                   if((finalp[0]=="qo2") && (stod(finalp[4]) > UmbralMinimoAmount))
                   {
                       Nodes[NOi].o2.push_back(Prod_parts(finalp[1],stod(finalp[4]),superm));
                       Nodes[NOi].o2.back().frescura = stoi(finalp[2]);
                   }
                   if((finalp[0]=="qu") && (stod(finalp[4]) > UmbralMinimoAmount))
                   {
                       Nodes[NOi].u.push_back(Prod_parts(finalp[1],stod(finalp[4]),superm));
                       Nodes[NOi].u.back().frescura = stoi(finalp[2]);
                   }
                   
               } // end caso de leer atributo para nodo oferta o demanda
               
           }   // end if recorriendo line

       }   // end while inputCUAD
    
    //actualizo atributos de los arcos, así se mantiene relación entre arcos
    G.Actualiza_arcos_compuestos(Edges, Nodes);
}       // end scan liink para caso de cuadratura


string obtenerVarNodo(map<string, map<string, string>> tipoNodoVar, string nodo, vector<string> tipos) {
    string rst = "";
	for (string e : tipos) {
		if (tipoNodoVar[e].find(nodo) != tipoNodoVar[e].end()) {
			rst = tipoNodoVar[e][nodo];
            break;
		}
	}
    return rst;
}

vector<string> obtenerTodosVarNodo(map<string, map<string, string>> tipoNodoVar, string nodo, vector<string> tipos) {
	vector<string> rst = vector<string>();
	for (string e : tipos) {
		if (tipoNodoVar[e].find(nodo) != tipoNodoVar[e].end()) {
			rst.push_back(tipoNodoVar[e][nodo]);
		}
	}
	return rst;
}

void grabarData(string nombre, vector<vector<string>> data) {
	fstream strm;
	strm.open("gdata.txt", ios_base::app);
	for (auto fila = data.begin(); fila != data.end(); fila++) {
		strm << nombre << ":";
		for (auto col = (*fila).begin(); col != (*fila).end(); col++) {
			strm << (*col) << ";";
		}
		strm << endl;
	}
	strm.close();
}

//ImportPython imp("c:\\Users\\Aul\\Desktop\\salida-P\\tmp\\exportCPP.txt");

set<string> cargaVectores(char *argv[], vector<Node>& Nodes, Graph& G, vector<Producto>& Prod, vector<Familia>& Fam, vector<Edge>& Edges) {
	ImportPython imp(argv[5]);
	vector<vector<string>> dataStock = imp.obtenerMatriz("dataStock"); //xlsx2Vector(argv[1], argv[2], {}, 5, 0, 1);
	vector<vector<string>> dataDemanda = imp.obtenerMatriz("dataDemanda"); //xlsx2Vector(argv[3], argv[4], {2, 4, 5, 6}, 0, 1, -1);
	vector<vector<string>> dataMateriales = imp.obtenerMatriz("dataMateriales"); //xlsx2Vector(argv[5], argv[6], { 0, 3, 18, 19, 20 }, 0, 1, -1);
	vector<vector<string>> dataFamilias = imp.obtenerMatriz("dataFamilias"); //xlsx2Vector(argv[7], argv[8], { 0, 1, 2, 3 }, 0, 1, -1);
	vector<vector<string>> dataCajasPallet = imp.obtenerMatriz("dataCajasPallet"); //xlsx2Vector(argv[9], argv[10], { 0, 2 }, 0, 1, -1);
	vector<vector<string>> dataCostoProduccion = imp.obtenerMatriz("dataCostoProduccion"); //xlsx2Vector(argv[11], argv[12], { 1, 3 }, 0, 1, -1);
	vector<vector<string>> dataPreciosVenta = imp.obtenerMatriz("dataPreciosVenta"); //xlsx2Vector(argv[13], argv[14], { 1, 3, 5 }, 0, 1, -1);
	vector<vector<string>> dataCV = imp.obtenerMatriz("dataCV"); //xlsx2Vector(argv[15], argv[16], { 0, 2, 4, 5 }, 0, 1, -1);
	vector<vector<string>> dataTransportes = imp.obtenerMatriz("dataTransportes"); //xlsx2Vector(argv[21], argv[22], { 0, 1, 2, 3, 4 }, 0, 1, -1);
	vector<vector<string>> dataNodos = imp.obtenerMatriz("dataNodos"); //xlsx2Vector(argv[21], argv[23], { 0, 1, 6, 7, 8, 9, 10, 11, 12 }, 0, 1, -1);
	vector<vector<string>> dataTViajes = imp.obtenerMatriz("dataTViajes"); //xlsx2Vector(argv[15], argv[24], { 0, 1, 2 }, 0, 1, -1);
	set<string> jreal;
	map<string, vector<string>> mapMateriales;
	map<string, DefinicionFamilia> mapDefinicionFamilia;
	map<string, vector<string>> mapCajasPallet;
	map<string, set<string>> mapDda;
	set<string> setSectores;
	map<string, vector<string>> mapNodos;
	map<string, double> mapTViajes;
	map<string, map<string, string>> mapTipoNodoVar;

	/*grabarData("dataStock", dataStock);
	grabarData("dataDemanda", dataDemanda);
	grabarData("dataMateriales", dataMateriales);
	grabarData("dataFamilias", dataFamilias);
	grabarData("dataCajasPallet", dataCajasPallet);
	grabarData("dataCostoProduccion", dataCostoProduccion);
	grabarData("dataPreciosVenta", dataPreciosVenta);
	grabarData("dataCV", dataCV);
	grabarData("dataTransportes", dataTransportes);
	grabarData("dataNodos", dataNodos);
	grabarData("dataTViajes", dataTViajes);

	exit(0);*/

	cout << "Cargando;" << endl;
	cout << " --> Nodos" << endl;
	vector<string> tipos = { "Pl", "Su", "Ve" };
	//////////////////////////////////////////////////////////
	/// Nodos
	//////////////////////////////////////////////////////////
	for (auto fila = dataNodos.begin(); fila != dataNodos.end(); fila++) {
		mapNodos.insert(make_pair((*fila)[1], *fila));
		string var = (*fila)[0].substr(0, 2) + (*fila)[1];
		if ((*fila)[0].compare("Hub") == 0) {
			var = "Pl" + (*fila)[1];
		}

		if (mapTipoNodoVar.find((*fila)[0]) == mapTipoNodoVar.end()) {
			mapTipoNodoVar.insert(make_pair((*fila)[0], map<string, string>()));
		}

		mapTipoNodoVar[(*fila)[0]].insert(make_pair((*fila)[1], var));
	}

	for (auto fila = dataTViajes.begin(); fila != dataTViajes.end(); fila++) {
		double dd = -1;
		try {
			dd = stof((*fila)[2]);
		}
		catch (exception& e)
		{
			cout << "Tiempo de viaje incorrecto: " << (*fila)[0] << ", " << (*fila)[1] << endl;
		}
		vector<string> eis = obtenerTodosVarNodo(mapTipoNodoVar, (*fila)[0], { "Hub", "Planta", "Sucursal", "Venta Directa" });
		vector<string> ejs = obtenerTodosVarNodo(mapTipoNodoVar, (*fila)[1], { "Hub", "Planta", "Sucursal", "Venta Directa" });
		for (auto ei = eis.begin(); ei != eis.end(); ei++) {
			for (auto ej = ejs.begin(); ej != ejs.end(); ej++) {
				if (((*ei).substr(0, 2).compare("Pl") == 0 && find(tipos.begin(), tipos.end(), (*ej).substr(0, 2)) != tipos.end())
					|| ((*ei).substr(0, 2).compare("Su") == 0 && (*ej).substr(0, 2).compare("Su") == 0)
					|| ((*ei).substr(0, 2).compare("Ve") == 0 && (*ej).substr(0, 2).compare("Ve") == 0)) {
					mapTViajes.insert(make_pair((*ei) + (*ej), dd));
				}
			}
		}
	}

	/// Carga nodos fijos
	Nodes.push_back(Node("io", 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0));
	Nodes.push_back(Node("jo", 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0));
	Nodes.push_back(Node("eo", 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0));

	/// Carga nodos origen
	for (auto cel = (*(dataStock.begin())).begin(); cel != (*(dataStock.begin())).end(); cel++) {
		string val = obtenerVarNodo(mapTipoNodoVar, (*cel), { "Hub", "Planta" });//mapNodoVarPl[*cel];
		//if (val.compare("60472") == 0)
		//	val = "P" + val;
		double la=-1, lo=-1, vA=-1, vB=-1;
		if (mapNodos[*cel][2].length() != 0) {
			la = stof(mapNodos[*cel][2]);
		}
		if (mapNodos[*cel][3].length() != 0) {
			lo = stof(mapNodos[*cel][3]);
		}
		if (mapNodos[*cel][4].length() != 0) {
			vA = stof(mapNodos[*cel][4]);
		}
		if (mapNodos[*cel][5].length() != 0) {
			vB = stof(mapNodos[*cel][5]);
		}
		Nodes.push_back(Node(val, 1, la, lo, vA, vB, stof(mapNodos[*cel][6]), stof(mapNodos[*cel][7]), stoi(mapNodos[*cel][8])));
	}
	/// Carga nodos destino
	for (auto fila = dataDemanda.begin(); fila != dataDemanda.end(); fila++) {
		jreal.insert((*fila)[1]);
	}
	for (auto it = jreal.begin(); it != jreal.end(); ++it) {
		string str = obtenerVarNodo(mapTipoNodoVar, (*it), { "Sucursal", "Venta Directa" });//mapNodoVarOt[*it];
		//replaceAll(str, "_", "");
		double la = -1, lo = -1, vA = -1, vB = -1;
		if (mapNodos[*it][2].length() != 0) {
			la = stof(mapNodos[*it][2]);
		}
		if (mapNodos[*it][3].length() != 0) {
			lo = stof(mapNodos[*it][3]);
		}
		if (mapNodos[*it][4].length() != 0) {
			vA = stof(mapNodos[*it][4]);
		}
		if (mapNodos[*it][5].length() != 0) {
			vB = stof(mapNodos[*it][5]);
		}
		Nodes.push_back(Node(str, 2, la, lo, vA, vB, stof(mapNodos[*it][6]), stof(mapNodos[*it][7]), stoi(mapNodos[*it][8])));
        
//        cout << " Node: " << str << "  Va: " << vA << " Vb: " << vB << endl;
        
	}

	G.setSize((int)Nodes.size());

	cout << " --> Demanda" << endl;
	//////////////////////////////////////////////////////////
	/// Demanda
	//////////////////////////////////////////////////////////
	for (auto fila = dataDemanda.begin(); fila != dataDemanda.end(); fila++) {
		//replaceAll((*fila)[1], "_", "");
		string str = obtenerVarNodo(mapTipoNodoVar, (*fila)[1], { "Sucursal", "Venta Directa" });
		int IdNodes = G.get_Node_id(str, Nodes);//G.get_Node_id(mapNodoVarOt[(*fila)[1]], Nodes);
		double fd;
		if ((*fila).size() < 4) {
			fd = 0;
		} else {
			try {
				fd = stod((*fila)[3]);
			} catch (const std::invalid_argument&) {
				fd = 0;
			}
		}
		if (IdNodes != -1) {
			Nodes[IdNodes].demanda.push_back(Prod_parts((*fila)[0], fd, (startsWith((*fila)[2], "CCS") && !endsWith((*fila)[2], "_2"))));
			if (mapDda.find((*fila)[0]) != mapDda.end()) {
				set<string> set = {};
				mapDda.insert(make_pair((*fila)[0], set));
			}
			mapDda[(*fila)[0]].insert(str);
		}
	}

	cout << " --> Productos/ Familias" << endl;
	//////////////////////////////////////////////////////////
	/// Productos/ Familias
	//////////////////////////////////////////////////////////
	for (auto fila = dataMateriales.begin(); fila != dataMateriales.end(); fila++) {
		mapMateriales.insert(make_pair((*fila)[0], *fila));
	}

	for (auto fila = dataFamilias.begin(); fila != dataFamilias.end(); fila++) {
		if (mapDefinicionFamilia.find((*fila)[0]) == mapDefinicionFamilia.end()) {
			DefinicionFamilia df;
			df.id = atoi((*fila)[0].c_str());
			df.sectores.insert((*fila)[2]);
			df.estados.insert((*fila)[1]);
			df.pos = (int)Fam.size();
			Fam.push_back(Familia(df.id));
			mapDefinicionFamilia.insert(make_pair((*fila)[0], df));
		} else {
			mapDefinicionFamilia[(*fila)[0]].sectores.insert((*fila)[2]);
			mapDefinicionFamilia[(*fila)[0]].estados.insert((*fila)[1]);
		}
	}

	for (auto fila = dataCajasPallet.begin(); fila != dataCajasPallet.end(); fila++) {
		mapCajasPallet.insert(make_pair((*fila)[0], *fila));
	}

	for (auto elem = mapMateriales.begin(); elem != mapMateriales.end(); elem++) {
		string cajas, peso;
		if (mapCajasPallet.find(elem->first) != mapCajasPallet.end()) {
			cajas = mapCajasPallet[elem->first][1];
		} else {
			cajas = "50";
		}
		if (elem->second[3].compare("(en blanco)") == 0) {
			peso = "100.0";
		}
		else {
			peso = elem->second[3];
		}

		if (mapDda.find(elem->first) != mapDda.end()) {
			/// Crea producctos
			Producto prod((int)Prod.size(), elem->first, peso, cajas, elem->second[1], elem->second[4]);
			setSectores.insert(elem->second[1]);

			// Asigna familia
			for (auto elemDF = mapDefinicionFamilia.begin(); elemDF != mapDefinicionFamilia.end(); elemDF++) {
				if (elemDF->second.estados.find(prod.get_estado()) != elemDF->second.estados.end() && elemDF->second.sectores.find(prod.get_sector()) != elemDF->second.sectores.end()) {
					prod.set_familia(elemDF->second.pos);
					// este codigo se puede eliminar...... y lo asociado a Fam....
					Fam[elemDF->second.pos].addp(elem->first);
					// ........
				}
			}
			Prod.push_back(prod);
		}
	}

	/// Asigna costo produccion
	for (auto fila = dataCostoProduccion.begin(); fila != dataCostoProduccion.end(); fila++) {
		int idp = get_Prod_id((*fila)[0], Prod);
		if (idp != -1) {
			Prod[idp].ReadInfoCF(stod((*fila)[1]));
		}
	}

	/// Asigna precios
	for (auto fila = dataPreciosVenta.begin(); fila != dataPreciosVenta.end(); fila++) {
		int idp = get_Prod_id((*fila)[0], Prod);
		//replaceAll((*fila)[1], "_", "");
		string str = obtenerVarNodo(mapTipoNodoVar, (*fila)[1], { "Sucursal", "Venta Directa" });
		if (idp != -1 && mapDda[(*fila)[0]].find(str) != mapDda[(*fila)[0]].end()) {
			Prod[idp].Precio.push_back(Prod_parts(str, stod((*fila)[2]), false));
		}
		/*else {
			cout << (*fila)[0] << ", " << (*fila)[1] << endl;
		}*/
	}

	cout << " --> Arcos" << endl;
	//////////////////////////////////////////////////////////
	/// Arcos
	//////////////////////////////////////////////////////////
	ifstream inArcos(argv[1]);
	if (inArcos.fail()){
		fprintf(stderr, "Error al abrir el archivo: %s.\n", argv[1]);
		exit(1);
	}
    
    if(!CUAD)
        scan_links(inArcos, G, Edges, Nodes, Prod);
	else {
		ifstream inArcosCUAD(argv[2]);
		if (inArcosCUAD.fail()){
			fprintf(stderr, "Error al abrir el archivo: %s.\n", argv[2]);
			exit(1);
		}
		scan_linksCUAD(inArcos, inArcosCUAD, G, Edges, Nodes, Prod);
	}

	for (auto edg = Edges.begin(); edg != Edges.end(); edg++) {
		(*edg).setTViaje(mapTViajes[Nodes[(*edg).v()].get_cod() + Nodes[(*edg).w()].get_cod()]);
//		cout << mapTViajes[Nodes[(*edg).v()].get_cod() + Nodes[(*edg).w()].get_cod()] << ";" << Nodes[(*edg).v()].get_cod() << ";" << Nodes[(*edg).w()].get_cod() << endl;
	}

	cout << " --> Costos Variables" << endl;
	//////////////////////////////////////////////////////////
	/// Costos Variables
	//////////////////////////////////////////////////////////
	//cout << "SIZE: " << dataTransportes.size() << endl;
	map<string, Transporte> trans;
	for (auto fila = dataTransportes.begin(); fila != dataTransportes.end(); fila++) {
		if ((*fila)[0].size() > 0) {
			struct Transporte tr;
			tr.tipo = (*fila)[0];
			tr.peso = stoi((*fila)[1]);
			tr.volumen = stoi((*fila)[2]);
			tr.minP = stoi((*fila)[3]);
			tr.maxP = stoi((*fila)[4]);
			(*fila)[0].erase(remove((*fila)[0].begin(), (*fila)[0].end(), ' '), (*fila)[0].end());
			trans[(*fila)[0]] = tr;
		}
	}

	for (auto fila = dataCV.begin(); fila != dataCV.end(); fila++) {
		vector<string> eis = obtenerTodosVarNodo(mapTipoNodoVar, (*fila)[0], { "Hub", "Planta", "Sucursal", "Venta Directa" });
		vector<string> ejs = obtenerTodosVarNodo(mapTipoNodoVar, (*fila)[1], { "Hub", "Planta", "Sucursal", "Venta Directa" });
		for (auto ei = eis.begin(); ei != eis.end(); ei++) {
			for (auto ej = ejs.begin(); ej != ejs.end(); ej++) {
				if (((*ei).substr(0, 2).compare("Pl") == 0 && find(tipos.begin(), tipos.end(), (*ej).substr(0, 2)) != tipos.end())
					|| ((*ei).substr(0, 2).compare("Su") == 0 && (*ej).substr(0, 2).compare("Su") == 0)
					|| ((*ei).substr(0, 2).compare("Ve") == 0 && (*ej).substr(0, 2).compare("Ve") == 0)) {
					string origen = (*ei);
					string destino = (*ej);

					//replaceAll(destino, "_", "");
					//replaceAll(origen, "_", "");
					//if (origen.compare("60472") == 0)
					//	origen = "P" + origen;

					int eid = G.checkEdge(origen, destino, Edges, Nodes);

					if (eid != -1)
					{
						//cout << (*fila)[2] << ": " << (*fila)[3] << endl;
						/*if ((*fila)[2].compare("26 P") == 0) {
							Edges[eid].setCostT1(stod((*fila)[3]));
							}
							else if ((*fila)[2].compare("12 P") == 0) {
							Edges[eid].setCostT2(stod((*fila)[3]));
							}*/
						(*fila)[2].erase(remove((*fila)[2].begin(), (*fila)[2].end(), ' '), (*fila)[2].end());
						Edges[eid].setCostT((*fila)[2], stod((*fila)[3]));
						Edges[eid].setTransportes(trans);
					}   // end if
				}
			}
		}

	}

	cout << "Ejecutando..." << endl;
	return setSectores;
}

#endif
