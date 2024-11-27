#include <map>
#include <vector>
#include <string>
#include <fstream>

#include <sstream>
#include <codecvt>


using namespace std;


std::wstring readFile(const char* filename)
{
	std::wifstream wif(filename);
	setlocale(LC_ALL, "cl_CL.UTF-8");
	wif.imbue(locale("cl_CL.UTF-8"));
	//wif.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
	std::wstringstream wss;
	wss << wif.rdbuf();
	return wss.str();
}

vector<string> split(const string& str, const string& delim, bool filaCompleta)
{
	vector<string> tokens;
	size_t prev = 0, pos = 0;
	do
	{
		pos = str.find(delim, prev);
		if (pos == string::npos) pos = str.length();
		string token = str.substr(prev, pos - prev);
		if (filaCompleta || !token.empty()) tokens.push_back(token);
		prev = pos + delim.length();
	} while (pos < str.length() && prev < str.length());
	return tokens;
}

class ImportPython {
private:
	map<string, vector<vector<string>>> mapData;

public:
	ImportPython(string nombreArchivo) {
		//ifstream archivo;
		//string linea;
		vector<vector<string>> dataStock, dataDemanda, dataMateriales, dataFamilias, dataCajasPallet, dataCostoProduccion, dataPreciosVenta, dataCV, dataTransportes, dataNodos, dataTViajes;
		
		mapData.insert(make_pair("dataStock", dataStock));
		mapData.insert(make_pair("dataDemanda", dataDemanda));
		mapData.insert(make_pair("dataMateriales", dataMateriales));
		mapData.insert(make_pair("dataFamilias", dataFamilias));
		mapData.insert(make_pair("dataCajasPallet", dataCajasPallet));
		mapData.insert(make_pair("dataCostoProduccion", dataCostoProduccion));
		mapData.insert(make_pair("dataPreciosVenta", dataPreciosVenta));
		mapData.insert(make_pair("dataCV", dataCV));
		mapData.insert(make_pair("dataTransportes", dataTransportes));
		mapData.insert(make_pair("dataNodos", dataNodos));
		mapData.insert(make_pair("dataTViajes", dataTViajes));
		
		//setup converter
		using convert_type = codecvt_utf8<wchar_t>;
		wstring_convert<convert_type, wchar_t> converter;

		//use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
		string converted_str = converter.to_bytes(readFile(nombreArchivo.c_str()));

		vector<string> lineas = split(converted_str, "\n", true);

		//archivo.open(nombreArchivo);
		//while (std::getline(archivo, linea)) {
		for(string linea: lineas) {
			//cout << linea;
			//getchar();
			vector<string> partes = split(linea, ":", true);
			vector<string> columnas = split(partes[1], ";", true);
			mapData[partes[0]].push_back(columnas);
		}
		//archivo.close();
	}

	vector<vector<string>> obtenerMatriz(string llave) {
		return mapData[llave];
	}
};