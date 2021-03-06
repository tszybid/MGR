
#include "pch.h"
//#include <opencv2/opencv.hpp> <- CarModel.hpp
#define _CRT_SECURE_NO_WARNINGS
#include<fstream>
#include<chrono>
//#include <omp.h> <- CarModel.hpp
//#include "CarModel.hpp" <- MultiframeModel.hpp
#include "MultiframeModel.hpp"

#include <iostream>
#include <fstream>
#include <iomanip> // to format image names using setw() and setfill()
#include <io.h>    // to check file existence using POSIX function access(). On Linux include <unistd.h>.
#include <set>

#include "Hungarian.h"
#include "KalmanTracker.h"

#include "opencv2/video/tracking.hpp"
#include "opencv2/highgui/highgui.hpp"

//#include "kamera_szwedzka.cpp"

/** USTAWIENIE NA 1 SPOWODUJE ZAPIS FILMU DO PLIKU
*/
#define ZAPIS 1
#define ZAPIS_TRAJEKTORII 0
#define ZAPIS_TRAJEKTORII_NIEWYGL 0
#define POKAZ_TRAJEKTORIE 1
#define POKAZ_MAPE 1

/** 0 WYLACZA WYSWIETLANIE NAPISOW
*/
#define VERBOSE 1

/** Maksymalne parametry sledzenia
*/
#define DIFF_MAX 130 //procent
#define DIST_MAX 70.0 //pikseli

/** Minimalna liczba klatek dla optymalizacji
*/
#define MIN_FRAMES 5

using namespace std;
using namespace cv;

typedef struct TrackingBox
{
	int frame;
	int id;
	Rect_<float> box;
}TrackingBox;

#define CNUM 20

void drawTraj(Mat& obraz, String filename, Mat macierzRotacji, Mat wektorTranslacji, Mat macierzKamery, Mat wspolczynnikiZnieksztalcen);
void drawTrajFromFile(Mat& obraz, String filename, Mat H);
double calcDistance(CarModel, CarModel);
double calcDistance(CarModel c1, Rect_<float> c2);
double calcMaskDifference(CarModel, CarModel);
bool testConditions(double difference, double distance);
double GetIOU(Rect_<float> bb_test, Rect_<float> bb_gt);
void TestSORT(string seqName, bool display, int poczatek, int koniec);
map<int, Rect_<float>> getTrackingData(int klatka);
CarModel matchTrackedModel(vector<CarModel> current_cars, Rect_<float> tracking_box);



int main()
{

	vector<Scalar> colors = { Scalar(255,0,0), Scalar(0,255,0), Scalar(0,0,255), Scalar(255,255,0), Scalar(0,255,255), Scalar(255,0,255),
								Scalar(123,0,0), Scalar(123,123,0), Scalar(0,123,255)};
	
	//KLATKA POCZATKOWA i KONCOWA
	const int poczatek = 0;
	int klatka = poczatek; 
	const int koniec = 600;
	
	

	//test



	//VideoCapture cap2("szwed.avi");

	
	/*
	vector<double> wek = { 1,5,4,7,8,3,1 };
	Mat m = Mat(wek);
	Mat result;

	GaussianBlur(m, result, Size(1, 3), 0);
	cout << result << endl;

	waitKey(500000);
	goto start;
	*/
	//


	//cout << getBuildInformation() << endl;
	//waitKey(2000000);

	VideoWriter video;
	if (ZAPIS) {
		video.open("out.avi", VideoWriter::fourcc('M', 'P', '4', 'V'), 15, Size(1920, 1080));
		if (!video.isOpened()) {
			return 1;
		}
	}
	VideoWriter video_mapa;
	if (ZAPIS) {
		video_mapa.open("mapa.avi", VideoWriter::fourcc('M', 'P', '4', 'V'), 15, Size(1827, 1496));
		if (!video_mapa.isOpened()) {
			return 1;
		}
	}
	VideoWriter video_mapa2;
	if (ZAPIS) {
		video_mapa2.open("mapa_porownanie.avi", VideoWriter::fourcc('M', 'P', '4', 'V'), 15, Size(800, 600));
		if (!video_mapa2.isOpened()) {
			return 1;
		}
	}
	srand((int)time(0));

	//const Mat macierzKamery = (Mat_<double>(3, 3) << 465 * 1920 / 768, 0, 409 * 1920 / 768, 0, 465 * 1920 / 768, 278 * 1920 / 768, 0, 0, 1);
	//Mat macierzKamery = (Mat_<double>(3, 3) << 465 * 1920 / 1080, 0, 409 * 1920 / 1080, 0, 465 * 1920 / 1080, 278 * 1920 / 1080, 0, 0, 1);

	//const Mat macierzKamery = (Mat_<double>(3, 3) << 1204.02, 0, 894.44, 0, 1204.02, 528.62, 0, 0, 1);
	

	//const Mat wspolczynnikiZnieksztalcen = (Mat_<double>(1, 5) << -0.38, 0.08, 0.022, 0.002, 0.01);


	//const Mat macierzRotacji = (Mat_<double>(3, 3) << 0.8263026493225752, -0.5607335873632568, 0.05293180258810737,
	//	-0.4278102153165463, -0.6859840870997903, -0.5885611709216619,
	//	0.3663363910305716, 0.463684888958655, -0.8067180253087791);


	//const Mat wektorTranslacji = (Mat_<double>(3, 1) << -11.12, 2.26, 12.33);

	// STARA KALIBRACJA
	//const Mat macierzKamery = (Mat_<double>(3, 3) << 1002.112321165211, 0, 987.2491830343009, 0, 1002.112321165211, 589.9918790864365, 0, 0, 1);

	//const Mat wspolczynnikiZnieksztalcen = (Mat_<double>(1, 8) << -1.169871933599445, 1.985415625735632, 0.002295466362434718, 0.001387136095156824, 0.3886998975085835, -0.7172880331551088, 1.350554757529225, 1.32558662080435);

	//const Mat macierzRotacji = (Mat_<double>(3, 3) << 0.8739170471767888, -0.4860698940058433, 0.002247842287823654, -0.3488397196803014, -0.6303945454768441, -0.6934793198116378, 0.3384964469934233, 0.6052592627743845, -0.7204730253095095);

	//const Mat wektorTranslacji = (Mat_<double>(3, 1) << -10.17717169045413, 3.79443858516614, 9.618133705259625);

	
	// NOWA KALIBRACJA 08.10.2020
	/*
	const Mat macierzKamery = (Mat_<double>(3, 3) << 1028.973391773702, 0, 985.3521413445968,
		0, 1028.973391773702, 589.6550073756332,
		0, 0, 1);

	const Mat wspolczynnikiZnieksztalcen = (Mat_<double>(1, 8) << 1.238622265206154, 1.814418712555929, 0.002815741026057055, 0.001371511131507414, 0.7367104791862827, 1.964033030708728, 1.670098891260038, 1.952178280943168);

	const Mat wektorTranslacji = (Mat_<double>(3, 1) << -10.17571492748712,
		3.796811108129032,
		9.726712350190484);

	const Mat macierzRotacji = (Mat_<double>(3, 3) << 0.8740545967417021, -0.4858273746851194, 0.0005691408127658226,
		-0.3498880512315189, -0.6302973288352385, -0.6930394136472854,
		0.3370562468195926, 0.605555149651729, -0.7208994709459579);
	*/
	
	// NAJNOWSZA KALIBRACJA 12.10.2020
	/*
	const Mat macierzKamery = (Mat_<double>(3, 3) << 978.2745235971795, 0, 964.6517096929971,
													0, 944.6196519318304, 589.9939316954108,
													0, 0, 1);

	const Mat wspolczynnikiZnieksztalcen = (Mat_<double>(1, 8) << 0.8324274077326189, 2.999999754993848, 0.002814397366204329, 0.001124833053583371, 1.35987254817043, 1.330364524807418, 2.925230499721684, 2.999848036740712);

	const Mat wektorTranslacji = (Mat_<double>(3, 1) << -9.762219541051071,
														4.248433666612367,
														9.422868318510135);

	const Mat macierzRotacji = (Mat_<double>(3, 3) << 0.8826495895871207, -0.469767010281331, -0.0157688951111703,
 -0.3605493323090034, -0.6551536613225926, -0.6639110324637885,
 0.3015524514462104, 0.5916862649301076, -0.7476452921806165);
	*/
	//NOWA KALIBRACJA 13.10.2020
	const Mat macierzKamery = (Mat_<double>(3, 3) << 1060.110459745811, 0, 969.5495542511281,
	0, 1090.02184615697, 569.1152750480022,
	0, 0, 1);

	const Mat wspolczynnikiZnieksztalcen = (Mat_<double>(1, 8) << -0.4324251772568071, 2.364132679815188, 0.008266725609657541, 0.001118370358531905, 1.556893153048111, 0.2409853472352748, 1.329097297275845, 3.677904323513058);

	const Mat wektorTranslacji = (Mat_<double>(3, 1) << -0.3553512619829435,
	-1.552620588350424,
	15.9149979698209);

	const Mat macierzRotacji = (Mat_<double>(3, 3) << 0.8811400433871717, -0.4727019129818488, -0.01204680052764304,
	-0.331550701349404, -0.5994611606427942, -0.7285056275112171,
	0.3371444147162614, 0.6459096053969894, -0.6849338838760534);

	Mat obraz(1080, 1920, CV_8UC3);
	Mat obraz_org;
	VideoCapture c("20171223_090600_4174.mkv");
	//VideoCapture c("szwed0.avi");
	VideoCapture c2("szwed1.avi");
	VideoCapture c_map("szwed_mapa.avi");
	//namedWindow("obraz", 0);
	//c.set(CAP_PROP_POS_FRAMES, 77);
	//c >> obraz;
	//imshow("1", obraz);
	//waitKey();

		//testy
	/*
	int i = 0;
	FileStorage fs2;
	while (1) {
		c.set(CAP_PROP_POS_FRAMES, i);
		c >> obraz;
		String plik = "xml\\frame" + to_string(i) + ".xml";
		fs2.open(plik, FileStorage::READ);
		Mat boxes2 = Mat_<int>(4, 1);
		fs2["bbox_" + to_string(0)] >> boxes2;

		vector<int> box2;
		boxes2.col(0).copyTo(box2);
		Rect roi2(box2[0], box2[1], box2[2] - box2[0], box2[3] - box2[1]);

		// wczytanie maski
		Mat mask2(roi2.height, roi2.width, CV_8UC1);
		fs2["mask_" + to_string(0)] >> mask2;
		mask2 = mask2 * 255;
		imshow("0", mask2);
		imshow("1", obraz);
		waitKey();
		i++;
	}*/
	/*
	c.set(CAP_PROP_POS_FRAMES, 1);
	c >> obraz;
	vector<Point3d> pkt = { {8.8 , 0.8 , 0}, { 0,0,0 }, { 10.610, -0.119, 0 }, { -6.330, 13.471, 0}, { -8.902, 22.576, 0} };
	vector<Point2d> pkt2;
	projectPoints(pkt, macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen, pkt2);
	int licz = 0;
	for (auto p : pkt2) {
		circle(obraz, p, 5, CV_RGB(0, 0, 0));
		putText(obraz, format("%2d", licz), p, 0, 1, CV_RGB(255, 0, 0), 1);
		licz++;
	}
	imshow("asdas", obraz);
	waitKey();
	*/

	vector<MultiframeModel> modele;

	TestSORT("aaa", 0, poczatek, koniec);
	//waitKey(1);

	int tracked_id = 0;
	FileStorage fs;


	for (klatka; klatka < koniec; klatka++) {
		cout << to_string(klatka) << endl;
		auto czas_poczatkowy = chrono::steady_clock::now();
		//c.set(CAP_PROP_POS_FRAMES, klatka);
		//c >> obraz;
		//obraz_org = obraz.clone();
		
		// plik xml z wynikami dzialania sieci (python)
		String plik;
		plik = "xml\\frame" + to_string(klatka) + ".xml";
		fs.open(plik, FileStorage::READ);
		// ilosc wykrytych samochodow
		int count;
		fs["count"] >> count;

		vector<CarModel> current_cars;
		
		//petla po wszystkich samochodach w danej klatce i ich zapis w wektorze

		for (int i = 0; i < count; i++) {

			// wczytanie roi
			Mat boxes = Mat_<int>(4, 1);
			fs["bbox_" + to_string(i)] >> boxes;

			vector<int> box;
			boxes.col(0).copyTo(box);
			Rect roi(box[0], box[1], box[2] - box[0], box[3] - box[1]);

			// wczytanie maski
			Mat mask(roi.height, roi.width, CV_8UC1);
			fs["mask_" + to_string(i)] >> mask;
			mask = mask * 255;
			//Mat binmask = mask.clone();

			CarModel Car = CarModel(roi, mask, macierzKamery, wspolczynnikiZnieksztalcen, macierzRotacji, wektorTranslacji);
			current_cars.push_back(Car);

		}
		fs.release();
		auto czas = chrono::steady_clock::now();
		cout << "Czas wczytywania: " << chrono::duration_cast<chrono::milliseconds>(czas - czas_poczatkowy).count() << "ms" << endl;

		//wczytanie wyniku sledzenia dla tej klatki
		map<int, Rect_<float>> tracking = getTrackingData(klatka);

		//Dodanie wyśledzonych samochodów do multimodeli lub utworzenie nowych dla nowych id
		for (auto& tr : tracking) {
			if (tr.first > tracked_id) {
				modele.push_back(MultiframeModel(matchTrackedModel(current_cars, tr.second), klatka, tr.first));
				tracked_id = tr.first;
				continue;
			}
			else {
				//znajdz multimodel w wektorze models o konkretnym id i dodaj do niego kolejny carmodel
				for (auto& mod : modele) {
					if (mod.track_id == tr.first) {
						mod.insert(matchTrackedModel(current_cars, tr.second), klatka);
						break;
					}
				}
			}
		}
		auto czas1 = chrono::steady_clock::now();
		cout << "Czas dopasowania: " << chrono::duration_cast<chrono::milliseconds>(czas1 - czas).count() << "ms" << endl;
		//waitKey();
		//continue;
		/*

		vector<int> rozpoznane;
		//Sledzenie pojazdow
		//Dla pierwszej klatki automatycznie wpisuje wszystkie modele do wektora
		if (modele.size() == 0) {
			for (int i = 0; i < current_cars.size(); i++) {
				modele.push_back(MultiframeModel(current_cars[i], klatka));
			}
		}
		//A dla pozostałych uruchamiam śledzenie
		else {
			for (int i = 0; i < current_cars.size(); i++) { //petla po wszystkich dotychczasowych wektorach modeli
				int j_min = -1; double diff_min = 30000.0, dist_min = 30000.0;
				//cout << "i = " << i << endl;
				for (int j = 0; j < modele.size(); j++) { //petla po wszystkich pojazdach z obecnej klatki	
					//cout << "j = " << j << endl;
					if (modele[j].models.count(klatka - 1) == 0 || modele[j].models.count(klatka) > 0 || modele[j].getStatus() == 0) continue; //sprawdzenie, czy w danej mapie jest model z poprzedniej klatki, czu juz zostal przypisany w tej petli i czy jest otwarty
					double diff = calcMaskDifference(modele[j].models[klatka - 1], current_cars[i]); //wielkosc maski tylko dla potwierdzenia, ze to ten sam pojazd
					double dist = calcDistance(modele[j].models[klatka - 1], current_cars[i]);
					//cout << diff << endl;
					//cout << dist << endl;
					if (testConditions(diff, dist) == false) continue; //sprawdzenie, czy nie przekroczono maksymalnych odleglosci
					if (dist < dist_min) { dist_min = dist; j_min = j; }
				}
				if (j_min == -1) { modele.push_back(MultiframeModel(current_cars[i], klatka)); } // nic nie rozpoznano -> dodajemy nowy multimodel do wektora
				else { modele[j_min].insert(current_cars[i], klatka); rozpoznane.push_back(j_min); } //a jesli rozpoznano, to dodajemy model najblizszego multimodelu
			}
		}
		
		//test - rysowanie
		/*
		cout << to_string(modele.size()) << endl;
		for (int i = 0; i < modele.size(); i++) {
			if (modele[i].models.count(klatka) > 0) modele[i].models[klatka].drawMask(obraz, colors[i%10]);
			for (int j = 0; j < modele[i].models.size()-1; j++) {
				line(obraz, Point(modele[i].models[klatka - j].getRoi().x, modele[i].models[klatka - j].getRoi().y), Point(modele[i].models[klatka - j - 1].getRoi().x, modele[i].models[klatka - j - 1].getRoi().y), Scalar(255, 0, 255));
			}
		}
		putText(obraz, to_string(modele.size()), Point(0, 100), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(255, 0, 0), 2);
		//waitKey();
		*/
		
		//Zamykanie nierozponanych modeli w tej klatce
		for (int i = 0; i < modele.size(); i++) {
			if (modele[i].models.count(klatka) == 0 || klatka == koniec-1) { //jesli dany multimodel nie zostal w danej klatce rozpoznany
				if (modele[i].getSize() < MIN_FRAMES) { modele.erase(modele.begin() + i); i--; }         // usuwamy modele ktore sa sledzone w 3 lub mniej klatkach
				else modele[i].close();            //to zamykamy
			}                  
		}

			/*
			Car.optimize(VERBOSE);
			// warunek, żeby nie rysować bardzo złych dopasowań - np. takich, które są kompletnie obok
			int koszt = Car.getOptimalCost();
			int maskarea = countNonZero(binmask);
			if (abs(koszt - maskarea) > 100) {
				cars[i].drawMask(obraz);
				cars[i].draw2(obraz);
			}
			*/
		//}
		
		auto czas_koncowy = chrono::steady_clock::now();

		if (VERBOSE) cout << "Czas optymalizacji: " << chrono::duration_cast<chrono::milliseconds>(czas_koncowy - czas_poczatkowy).count() << "ms" << endl;
		
	}
	//goto wyswietlanie;
	//po przetworzeniu calego zakresu klatek:

	//for (int i = 0; i < modele.size(); i++) cout << modele[i].getSize() << endl;;
	//cout << "=========================" << endl;
	
	//wyrzucenie samochodow rozpoznanych w mniej niz 3 klatkach
	for (int i = 0; i < modele.size(); i++) {
		if (modele[i].getSize() < MIN_FRAMES) {
			modele.erase(modele.begin() + i);
			i--;
		}
	}

	//for (int i = 0; i < modele.size(); i++) cout << modele[i].getSize() << endl;;
	//waitKey();
 #pragma omp parallel for
	for (int i = 0; i < modele.size(); i++) {
		if (modele[i].getStatus() == 0) modele[i].optimizeModels();
	}

	vector<MultiframeModel> modele_kopia = modele;
	//wygladzanie
	for (auto& samochod : modele) {
		vector<double> katy, wynik;
		vector<double> iksy, wynik2;
		vector<double> igreki, wynik3;
		for (auto& m : samochod.models) {
			katy.push_back(double(m.second.getAngle()));
			iksy.push_back(double(m.second.getX()));
			igreki.push_back(double(m.second.getY()));
		}
		Mat src = Mat(katy);
		Mat src2 = Mat(iksy);
		Mat src3 = Mat(igreki);
		Mat dst, dst2, dst3;
		GaussianBlur(src, dst, Size(1, 15), 0);
		GaussianBlur(src2, dst2, Size(1, 15), 0);
		GaussianBlur(src3, dst3, Size(1, 15), 0);
		dst.col(0).copyTo(wynik);
		dst2.col(0).copyTo(wynik2);
		dst3.col(0).copyTo(wynik3);
		int i = 0;
		for (auto& m : samochod.models) {
			m.second.setAngle(int(wynik[i]));
			m.second.setX(float(wynik2[i]));
			m.second.setY(float(wynik3[i]));
			m.second.buildModel2();
			i++;
		}
	}



	//Mat H = (Mat_<double>(3, 3) << 0.5579473078814168, 30.16195971205332, 206.6370680774399,
	//	30.18296347770574, -0.5498443047120708, 141.0443978971229,
	//	-3.590713167129858e-05, -5.187437584337256e-05, 1);

	// NOWE H 12.10.2020
	//Mat H = (Mat_<double>(3, 3) << 0.560531657694993, 30.18131945800637, 188.7206819789358,
	//30.31241203732241, -0.5465808427327407, 100.6111117731869,
	//8.370599135039687e-05, -6.415278638680027e-05, 1);

	// NOWE H 13.10.2020
	Mat H = (Mat_<double>(3, 3) << 0.3468467851761252, 29.96772201128925, 282.2854540188393,
	30.12091765291587, -0.628145699285484, 468.5137076056328,
	-0.0001317553779147852, -0.0001649824901583339, 1);

wyswietlanie:
	klatka = poczatek;
	for (klatka; klatka < koniec; klatka++) {
		/*
		if (klatka < 260) {
			c.set(CAP_PROP_POS_FRAMES, klatka);
			c >> obraz;
		}
		else if (klatka >= 260 && klatka < 310)
		{
			c.set(CAP_PROP_POS_FRAMES, klatka-4);
			c >> obraz;
		}
		else
		{
			c2.set(CAP_PROP_POS_FRAMES, klatka-4);
			c2 >> obraz;
		}
		*/
		Mat obraz_map;
		
		c.set(CAP_PROP_POS_FRAMES, klatka);
		c >> obraz;
		
		c_map.set(CAP_PROP_POS_FRAMES, klatka);
		c_map >> obraz_map;
		
		Mat map = imread("mapa.png", IMREAD_COLOR);
		/*//STARA //////////////////////////////
		for (auto samochod : modele_kopia) {
			if (samochod.models.count(klatka) != 0 && samochod.getStatus() == 0) {
				samochod.models[klatka].draw2(obraz);
				//samochod.models[klatka].drawMask(obraz, colors[samochod.track_id % 10]);
				Point ten_samochod(samochod.models[klatka].getRoi().x, samochod.models[klatka].getRoi().y);
				putText(obraz, to_string(samochod.track_id), ten_samochod, FONT_HERSHEY_COMPLEX, 3, Scalar(0, 0, 255));
				if (POKAZ_TRAJEKTORIE) {
					if (samochod.models.count(klatka - 1) != 0) {
						for (auto& m : samochod.models) {

							if (m.first == samochod.findMaxFrame()) break;
							vector<Point3f> p1 = { {m.second.getX(), m.second.getY(), 0}, {samochod.models[m.first + 1].getX(), samochod.models[m.first + 1].getY(), 0} };
							vector<Point2f> p2;
							projectPoints(p1, macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen, p2);
							line(obraz, p2[0], p2[1], Scalar(0, 0, 255), 1);

							if (POKAZ_MAPE) {
								vector<Point3d> p = { {m.second.getX(), m.second.getY(), 1}, {samochod.models[m.first + 1].getX(), samochod.models[m.first + 1].getY(), 1} };
								Point3d pm = Point3d(Mat(H * Mat(p[0])));
								pm /= pm.z;
								Point3d pm2 = Point3d(Mat(H * Mat(p[1])));
								pm2 /= pm2.z;
								line(map, Point(pm.x, pm.y), Point(pm2.x, pm2.y), Scalar(0, 0, 255), 1);


							}
						}
					}
				}
				if (POKAZ_MAPE) {
					samochod.models[klatka].drawOnMap(map);
					Point3d p = Point3d(samochod.models[klatka].getX(), samochod.models[klatka].getY(), 1);
					Point3d pm = Point3d(Mat(H * Mat(p)));
					pm /= pm.z;
					putText(map, to_string(samochod.track_id), Point(pm.x, pm.y), FONT_HERSHEY_COMPLEX, 2, Scalar(255, 0, 0));
				}
			}
		}
		////////////////////////*/
		for (auto samochod : modele) {
			if (samochod.models.count(klatka) != 0 && samochod.getStatus() == 0) {
				samochod.models[klatka].draw2(obraz);
				//samochod.models[klatka].drawMask(obraz, colors[samochod.track_id%10]);
				Point ten_samochod(samochod.models[klatka].getRoi().x, samochod.models[klatka].getRoi().y);
				putText(obraz, to_string(samochod.track_id), ten_samochod, FONT_HERSHEY_COMPLEX, 3, Scalar(0, 0, 255));
				if (POKAZ_TRAJEKTORIE) {
					if (samochod.models.count(klatka - 1) != 0) {
						for (auto& m : samochod.models) {

							if (m.first == samochod.findMaxFrame()) break;
							vector<Point3f> p1 = { {m.second.getX(), m.second.getY(), 0}, {samochod.models[m.first + 1].getX(), samochod.models[m.first + 1].getY(), 0} };
							vector<Point2f> p2;
							//projectPoints(p1, macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen, p2);
							projectPoints2(p1, p2);
							line(obraz, p2[0], p2[1], Scalar(0, 0, 0), 4);
							//circle(obraz, p2[0], 3, CV_RGB(0, 0, 0));

							if (POKAZ_MAPE) {
								vector<Point3d> p = { {m.second.getY(), m.second.getX(), 1}, {samochod.models[m.first + 1].getY(), samochod.models[m.first + 1].getX(), 1} };
								Point3d pm = Point3d(Mat(H * Mat(p[0])));
								pm /= pm.z;
								Point3d pm2 = Point3d(Mat(H * Mat(p[1])));
								pm2 /= pm2.z;
								line(map, Point(pm.x, pm.y), Point(pm2.x, pm2.y), Scalar(0, 0, 0), 4);
								line(obraz_map, Point(pm.x, pm.y), Point(pm2.x, pm2.y), Scalar(0, 0, 0), 4);
								//circle(map, Point(pm.x, pm.y), 5, CV_RGB(0, 0, 0));


							}
						}
					}
				}
				if (POKAZ_MAPE) {
					samochod.models[klatka].drawOnMap(map, H);
					//samochod.models[klatka].drawOnMap(obraz_map, H);
					Point3d p = Point3d(samochod.models[klatka].getY(), samochod.models[klatka].getX(), 1);
					Point3d pm = Point3d(Mat(H * Mat(p)));
					pm /= pm.z;
					putText(map, to_string(samochod.track_id), Point(pm.x,pm.y), FONT_HERSHEY_COMPLEX, 2, Scalar(255, 0, 0));
					putText(obraz_map, to_string(samochod.track_id), Point(pm.x, pm.y), FONT_HERSHEY_COMPLEX, 2, Scalar(255, 0, 0));
				}
			}
		}
		if (klatka > 46 && klatka < 136) drawTraj(obraz, "RoadUser_1.txt", macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen);
		if (klatka > 76 && klatka < 196) drawTraj(obraz, "RoadUser_2.txt", macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen);
		if (klatka > 135 && klatka < 195) drawTraj(obraz, "RoadUser_3.txt", macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen);
		if (klatka > 246 && klatka < 306) drawTraj(obraz, "RoadUser_4.txt", macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen);
		//if (klatka > 730 && klatka < 870) drawTraj(obraz, "RoadUser_5.txt", macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen);
		//if (klatka > 850 && klatka < 920) drawTraj(obraz, "RoadUser_6.txt", macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen);
		//if (klatka > 1200 && klatka < 1320) drawTraj(obraz, "RoadUser_7.txt", macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen);
		//if (klatka > 2500 && klatka < 2570) drawTraj(obraz, "RoadUser_8.txt", macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen);

		/*
		//testy
		vector<Point3d> pkt = { {8.811 , 0.832 , 0}, {8.899 , 1.141 , 0}, {-7.336 , 16.042 , 0}, {-7.506 , 16.662 , 0}, { 0,0,0 }, { 10.610, -0.119, 0 }, { -6.330, 13.471, 0}, { -8.902, 22.576, 0} };
		vector<Point2d> pkt2;
		projectPoints(pkt, macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen, pkt2);
		int licz = 0;
		for (auto p : pkt2) {
			circle(obraz, p, 5, CV_RGB(0, 0, 0));
			putText(obraz, format("%2d", licz), p, 0, 1, CV_RGB(255, 0, 0), 1);
			licz++;
		}
		*/

		if (ZAPIS) {
			video.write(obraz);
		}
		if (ZAPIS && POKAZ_MAPE) {
			if (klatka > 46 && klatka < 136) drawTrajFromFile(map, "RoadUser_1.txt", H);
			if (klatka > 76 && klatka < 196) drawTrajFromFile(map, "RoadUser_2.txt", H);
			if (klatka > 135 && klatka < 195) drawTrajFromFile(map, "RoadUser_3.txt", H);
			if (klatka > 246 && klatka < 306) drawTrajFromFile(map, "RoadUser_4.txt", H);
			//if (klatka > 730 && klatka < 870) drawTrajFromFile(obraz, "RoadUser_5.txt", H);
			if (klatka > 850 && klatka < 920) drawTrajFromFile(map, "RoadUser_6.txt", H);
			if (klatka > 1200 && klatka < 1320) drawTrajFromFile(map, "RoadUser_7.txt", H);
			if (klatka > 2500 && klatka < 2570) drawTrajFromFile(map, "RoadUser_8.txt", H);

			//drawTrajFromFile(map, "trajek2.txt", H);
			//drawTrajFromFile(map, "trajek3.txt", H);
			//drawTrajFromFile(obraz_map, "trajek2.txt", H);
			//drawTrajFromFile(obraz_map, "trajek3.txt", H);
			video_mapa.write(map);
			//video_mapa2.write(obraz_map);
		}
		//imshow("obraz", obraz);
		waitKey(1);

	}

	//zapis trajektorii do plikow csv
	if (ZAPIS_TRAJEKTORII) {
		for (auto& model : modele) {
			ofstream resultsFile;
			string resFileName = "trajektoria_" + to_string(model.track_id) + ".csv";
			resultsFile.open(resFileName);
			resultsFile << "nr_klatki;x;y;angle;d;s;w" << endl;
			for (auto& m : model.models) {
				resultsFile << m.first << ";" << m.second.getX() << ";" << m.second.getY() << ";" << m.second.getAngle() << ";" << m.second.getD() << ";" << m.second.getS() << ";" << m.second.getW() << endl;
			}
			resultsFile.close();
		}
	}
	if (ZAPIS_TRAJEKTORII_NIEWYGL) {
		for (auto& model : modele_kopia) {
			ofstream resultsFile;
			string resFileName = "traj_niewygladzona_" + to_string(model.track_id) + ".csv";
			resultsFile.open(resFileName);
			resultsFile << "nr_klatki;x;y;angle;d;s;w" << endl;
			for (auto& m : model.models) {
				resultsFile << m.first << ";" << m.second.getX() << ";" << m.second.getY() << ";" << m.second.getAngle() << ";" << m.second.getD() << ";" << m.second.getS() << ";" << m.second.getW() << endl;
			}
			resultsFile.close();
		}
	}

	//zakonczenie
	destroyAllWindows();
	c.release();
	c2.release();
	video.release();
	return 0;
}

void drawTraj(Mat& obraz, String filename, Mat macierzRotacji, Mat wektorTranslacji, Mat macierzKamery, Mat wspolczynnikiZnieksztalcen)
{
	ifstream file;
	file.open(filename);

	while (!file.is_open());

	string detLine;
	istringstream ss;
	vector<Point3d> points;
	vector<Point2d> points2;
	char ch;
	double x, y, z, temp;


	while (getline(file, detLine))
	{
		ss.str(detLine);
		ss >> temp >> ch >> x >> ch >> y >> ch >> z >> ch;
		points.push_back(Point3d(x, y, z));
	}
	//projectPoints(points, macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen, points2);
	projectPoints2(points, points2);
	for (int i = 1; i < points2.size(); i++) {
		line(obraz, Point(points2[i-1].x, points2[i-1].y), Point(points2[i].x, points2[i].y), Scalar(0, 255, 0), 4);
	}

	for (int i = 0; i < points2.size(); i++)
	{
		//circle(obraz, points2[i], 3, CV_RGB(0, 255, 0));
		//putText(obraz, format("%2d", i++), points2[i], 0, 1, CV_RGB(255, 0, 0), 1);
		//putText(obraz, format("%.1f, %.1f", points2[i].x, points2[i].y), points2[i] + Point2d(-20, 20), 0, .5, CV_RGB(0, 0, 255), 1);
	}
}

void drawTrajFromFile(Mat& obraz, String filename, Mat H)
{
	ifstream file;
	file.open(filename);

	while (!file.is_open());

	string detLine;
	istringstream ss;
	vector<Point3d> points;
	char ch;
	double x, y, z, temp;

	while (getline(file, detLine))
	{
		ss.str(detLine);
		ss >> temp >> ch >> y >> ch >> x >> ch;
		points.push_back(Point3d(x, y, 1));
	}

	for (int i = 0; i < points.size()-1; i++) {
		Point3d pm = Point3d(Mat(H * Mat(points[i])));
		pm /= pm.z;
		Point3d pm2 = Point3d(Mat(H * Mat(points[i+1])));
		pm2 /= pm2.z;
		line(obraz, Point(pm.x, pm.y), Point(pm2.x, pm2.y), Scalar(0, 255, 0), 4);
	}

	for (int i = 0; i < points.size(); i++)
	{
		Point3d pm = Point3d(Mat(H * Mat(points[i])));
		pm /= pm.z;
		//circle(obraz, Point(pm.x, pm.y), 5, CV_RGB(255, 0, 0));
		//putText(obraz, format("%2d", i++), Point(pm.x, pm.y), 0, 1, CV_RGB(255, 0, 0), 2);
		//putText(obraz, format("%.1f, %.1f", pm.x, pm.y), Point2d(pm.x, pm.y) + Point2d(-20, 20), 0, .5, CV_RGB(0, 0, 255), 1);
	}
}

map<int, Rect_<float>> getTrackingData(int klatka)
{
	ifstream detectionFile;
	detectionFile.open("output.txt");

	while (!detectionFile.is_open());

	string detLine;
	istringstream ss;
	map<int, Rect_<float>> tracking;
	char ch;

	while (getline(detectionFile, detLine))
	{
		int frame = -1, id = -1;
		ss.str(detLine);
		ss >> frame >> ch >> id >> ch;
		if (frame < klatka) {
			ss.str(""); continue;
		}
		else if (frame > klatka) {
			ss.str(""); break;
		}
		else {
			float tpx, tpy, tpw, tph;
			ss >> tpx >> ch >> tpy >> ch >> tpw >> ch >> tph;
			ss.str("");

			Rect_<float> box = Rect_<float>(Point_<float>(tpx, tpy), Point_<float>(tpx + tpw, tpy + tph));
			tracking[id] = box;
		}
	}
	detectionFile.close();
	return tracking;
}

CarModel matchTrackedModel(vector<CarModel> current_cars, Rect_<float> tracking_box)
{
	int i_min = -1, i = 0;
	double dist_min = 50000;
	for (auto car : current_cars) {
		double dist = calcDistance(car, tracking_box);
		if (dist < dist_min) {
			i_min = i;
			dist_min = dist;
		}
		i++;
	}
	return current_cars[i_min];
}

double calcDistance(CarModel c1, CarModel c2) {
	return sqrt(double( pow(c2.getRoi().x - c1.getRoi().x, 2) + pow(c2.getRoi().y - c1.getRoi().y, 2) ) );
}

double calcDistance(CarModel c1, Rect_<float> c2) {
	return sqrt(double(pow(c2.x - c1.getRoi().x, 2) + pow(c2.y - c1.getRoi().y, 2)));
}

double calcMaskDifference(CarModel c1, CarModel c2) {
	double m1 = double(countNonZero(c1.getMask()));
	double m2 = double(countNonZero(c2.getMask()));
	if (m1 > m2) return m1 / m2 * 100.0;
	else return m2 / m1 * 100.0;
}

bool testConditions(double difference, double distance)
{
	if (difference < DIFF_MAX && distance < DIST_MAX) return true;
	else return false;
}

double GetIOU(Rect_<float> bb_test, Rect_<float> bb_gt)
{
	float in = (bb_test & bb_gt).area();
	float un = bb_test.area() + bb_gt.area() - in;

	if (un < DBL_EPSILON)
		return 0;

	return (double)(in / un);
}

void TestSORT(string seqName, bool display, int poczatek, int koniec)
{
	//cout << "Processing " << seqName << "..." << endl;

	// 0. randomly generate colors, only for display
	RNG rng(0xFFFFFFFF);
	Scalar_<int> randColor[CNUM];
	for (int i = 0; i < CNUM; i++)
		rng.fill(randColor[i], RNG::UNIFORM, 0, 256);

	string imgPath = "D:/Data/Track/2DMOT2015/train/" + seqName + "/img1/";

	if (display)
		if (_access(imgPath.c_str(), 0) == -1)
		{
			cerr << "Image path not found!" << endl;
			display = false;
		}

	// 1. read detection file
	//ifstream detectionFile;
	//string detFileName = "data/" + seqName + "/det.txt";
	//detectionFile.open(detFileName);

	// plik xml z wynikami dzialania sieci (python)

	string detLine;
	istringstream ss;
	vector<TrackingBox> detData;
	char ch;
	float tpx, tpy, tpw, tph;
	int klatka = poczatek;
	int randidx = 1;
	//while (getline(detectionFile, detLine))
	for(klatka; klatka < koniec; klatka++)
	{
		String plik;
		plik = "xml\\frame" + to_string(klatka) + ".xml";
		FileStorage fs = FileStorage(plik, FileStorage::READ);
		// ilosc wykrytych samochodow
		int count;
		fs["count"] >> count;

		for (int i = 0; i < count; i++) {
			TrackingBox tb;
			tb.frame = klatka;
			tb.id = randidx; randidx++;
			// wczytanie roi
			Mat boxes = Mat_<int>(4, 1);
			fs["bbox_" + to_string(i)] >> boxes;
			vector<int> box;
			boxes.col(0).copyTo(box);
			tb.box = Rect_<float>(Point_<float>(box[0], box[1]), Point_<float>(box[2], box[3]));
			//Rect roi(box[0], box[1], box[2] - box[0], box[3] - box[1]);
			detData.push_back(tb);
		}
		//tb.box = Rect_<float>(Point_<float>(tpx, tpy), Point_<float>(tpx + tpw, tpy + tph));
		fs.release();
	}
	//detectionFile.close();
	
	// 2. group detData by frame
	//int maxFrame = 0;
	int maxFrame = koniec;
	//for (auto tb : detData) // find max frame number
	//{
	//	if (maxFrame < tb.frame)
	//		maxFrame = tb.frame;
	//}

	vector<vector<TrackingBox>> detFrameData;
	vector<TrackingBox> tempVec;
	for (int fi = 0; fi < maxFrame; fi++)
	{
		for (auto tb : detData)
			if (tb.frame == fi + 1) // frame num starts from 1
				tempVec.push_back(tb);
		detFrameData.push_back(tempVec);
		tempVec.clear();
	}

	// 3. update across frames
	int frame_count = 0;
	int max_age = 1;
	int min_hits = 3;
	double iouThreshold = 0.05;
	vector<KalmanTracker> trackers;
	KalmanTracker::kf_count = 0; // tracking id relies on this, so we have to reset it in each seq.

	// variables used in the for-loop
	vector<Rect_<float>> predictedBoxes;
	vector<vector<double>> iouMatrix;
	vector<int> assignment;
	set<int> unmatchedDetections;
	set<int> unmatchedTrajectories;
	set<int> allItems;
	set<int> matchedItems;
	vector<cv::Point> matchedPairs;
	vector<TrackingBox> frameTrackingResult;
	unsigned int trkNum = 0;
	unsigned int detNum = 0;

	double cycle_time = 0.0;
	int64 start_time = 0;

	// prepare result file.
	ofstream resultsFile;
	string resFileName = "output.txt";
	resultsFile.open(resFileName);

	if (!resultsFile.is_open())
	{
		cerr << "Error: can not create file " << resFileName << endl;
		return;
	}

	//////////////////////////////////////////////
	// main loop
	for (int fi = 0; fi < maxFrame; fi++)
	{
		//total_frames++;
		frame_count++;
		//cout << frame_count << endl;

		// I used to count running time using clock(), but found it seems to conflict with cv::cvWaitkey(),
		// when they both exists, clock() can not get right result. Now I use cv::getTickCount() instead.
		start_time = getTickCount();

		if (trackers.size() == 0) // the first frame met
		{
			// initialize kalman trackers using first detections.
			for (unsigned int i = 0; i < detFrameData[fi].size(); i++)
			{
				KalmanTracker trk = KalmanTracker(detFrameData[fi][i].box);
				trackers.push_back(trk);
			}
			// output the first frame detections
			int modd = 0;
			FileStorage fss;
			for (unsigned int id = 0; id < detFrameData[fi].size(); id++)
			{
				TrackingBox tb = detFrameData[fi][id];
				resultsFile << tb.frame << "," << id + 1 << "," << tb.box.x << "," << tb.box.y << "," << tb.box.width << "," << tb.box.height << "," << endl;
			
				/*String plik1;
				plik1 = "xml\\frame" + to_string(tb.frame) + ".xml";
				fss.open(plik1, FileStorage::APPEND);
				fss << "id_" + to_string(modd) << tb.id;
				modd++;*/
			}
			continue;
		}

		///////////////////////////////////////
		// 3.1. get predicted locations from existing trackers.
		predictedBoxes.clear();

		for (auto it = trackers.begin(); it != trackers.end();)
		{
			Rect_<float> pBox = (*it).predict();
			if (pBox.x >= 0 && pBox.y >= 0)
			{
				predictedBoxes.push_back(pBox);
				it++;
			}
			else
			{
				it = trackers.erase(it);
				//cerr << "Box invalid at frame: " << frame_count << endl;
			}
		}

		///////////////////////////////////////
		// 3.2. associate detections to tracked object (both represented as bounding boxes)
		// dets : detFrameData[fi]
		trkNum = predictedBoxes.size();
		detNum = detFrameData[fi].size();

		iouMatrix.clear();
		iouMatrix.resize(trkNum, vector<double>(detNum, 0));

		for (unsigned int i = 0; i < trkNum; i++) // compute iou matrix as a distance matrix
		{
			for (unsigned int j = 0; j < detNum; j++)
			{
				// use 1-iou because the hungarian algorithm computes a minimum-cost assignment.
				iouMatrix[i][j] = 1 - GetIOU(predictedBoxes[i], detFrameData[fi][j].box);
			}
		}

		// solve the assignment problem using hungarian algorithm.
		// the resulting assignment is [track(prediction) : detection], with len=preNum
		HungarianAlgorithm HungAlgo;
		assignment.clear();
		HungAlgo.Solve(iouMatrix, assignment);

		// find matches, unmatched_detections and unmatched_predictions
		unmatchedTrajectories.clear();
		unmatchedDetections.clear();
		allItems.clear();
		matchedItems.clear();

		if (detNum > trkNum) //	there are unmatched detections
		{
			for (unsigned int n = 0; n < detNum; n++)
				allItems.insert(n);

			for (unsigned int i = 0; i < trkNum; ++i)
				matchedItems.insert(assignment[i]);

			set_difference(allItems.begin(), allItems.end(),
				matchedItems.begin(), matchedItems.end(),
				insert_iterator<set<int>>(unmatchedDetections, unmatchedDetections.begin()));
		}
		else if (detNum < trkNum) // there are unmatched trajectory/predictions
		{
			for (unsigned int i = 0; i < trkNum; ++i)
				if (assignment[i] == -1) // unassigned label will be set as -1 in the assignment algorithm
					unmatchedTrajectories.insert(i);
		}
		else
			;

		// filter out matched with low IOU
		matchedPairs.clear();
		for (unsigned int i = 0; i < trkNum; ++i)
		{
			if (assignment[i] == -1) // pass over invalid values
				continue;
			if (1 - iouMatrix[i][assignment[i]] < iouThreshold)
			{
				unmatchedTrajectories.insert(i);
				unmatchedDetections.insert(assignment[i]);
			}
			else
				matchedPairs.push_back(cv::Point(i, assignment[i]));
		}

		///////////////////////////////////////
		// 3.3. updating trackers

		// update matched trackers with assigned detections.
		// each prediction is corresponding to a tracker
		int detIdx, trkIdx;
		for (unsigned int i = 0; i < matchedPairs.size(); i++)
		{
			trkIdx = matchedPairs[i].x;
			detIdx = matchedPairs[i].y;
			trackers[trkIdx].update(detFrameData[fi][detIdx].box);
		}

		// create and initialise new trackers for unmatched detections
		for (auto umd : unmatchedDetections)
		{
			KalmanTracker tracker = KalmanTracker(detFrameData[fi][umd].box);
			trackers.push_back(tracker);
		}

		// get trackers' output
		frameTrackingResult.clear();
		for (auto it = trackers.begin(); it != trackers.end();)
		{
			if (((*it).m_time_since_update < 1) &&
				((*it).m_hit_streak >= min_hits || frame_count <= min_hits))
			{
				TrackingBox res;
				res.box = (*it).get_state();
				res.id = (*it).m_id + 1;
				res.frame = frame_count;
				frameTrackingResult.push_back(res);
				it++;
			}
			else
				it++;

			// remove dead tracklet
			if (it != trackers.end() && (*it).m_time_since_update > max_age)
				it = trackers.erase(it);
		}

		//cycle_time = (double)(getTickCount() - start_time);
		//total_time += cycle_time / getTickFrequency();
		int mod = 0;
		FileStorage fs;
		for (auto tb : frameTrackingResult) {
			resultsFile << tb.frame << "," << tb.id << "," << tb.box.x << "," << tb.box.y << "," << tb.box.width << "," << tb.box.height << "," << endl;

			/*String plik1;
			plik1 = "xml\\frame" + to_string(tb.frame) + ".xml";
			fs.open(plik1, FileStorage::APPEND);
			fs << "id_" + to_string(mod) << tb.id;
			mod++;*/
		}
		if (display) // read image, draw results and show them
		{
			ostringstream oss;
			oss << imgPath << setw(6) << setfill('0') << fi + 1;
			Mat img = imread(oss.str() + ".jpg");
			if (img.empty())
				continue;

			for (auto tb : frameTrackingResult)
				cv::rectangle(img, tb.box, randColor[tb.id % CNUM], 2, 8, 0);
			imshow(seqName, img);
			waitKey(40);
		}
	}

	resultsFile.close();

	if (display)
		destroyAllWindows();
}



/*

	//prostopadloscian, wspolrzedne i wymiary
	float x = 0, y = 0, z = 0, s = 1.5, d = 4.0, w = 1.2, a = 0.5, b = 0.5;
	const float s_start = 1.5, d_start = 4.0, w_start = 1.2, a_start = 0.5, b_start = 0.5;
	//float x_start, y_start;
	int angle = 0;
	int angle_opt[36];
	int x_opt[11], y_opt[11], d_opt[16], s_opt[6], w_opt[11], a_opt[6], b_opt[8];

	//punkt poczatkowy modelu
	//vector<Point3f> p1{ { x, y, z }, {x, y, z + w} };
	vector<Point3f> p1{ { x, y, z } };
	vector<Point2f> p2;

	//rzutowanie punktu na plaszczyzne obrazu
	projectPoints(p1, macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen, p2);
	
	//ustawienie modelu w okolicy ROI - punkt poczatkowy modelu
	//przesuwanie punktow w 3d, i sprawdzanie ich wspolrzednych w 2d, czy zgadzaja sie z poczatkiem roi
	while (int(p2[0].x) < xroi) {
		p1[0].x += .1;
		p1[0].y -= .1;
		projectPoints(p1, macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen, p2);
		while (p2[0].y > yroi + wys) {
			p1[0].y += .1;
			projectPoints(p1, macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen, p2);
		}
	}
	// zwiekszanie o 0.5 zeby od razu przesunac blizej srodka roi
	float xs = p1[0].x + 0.5;
	float ys = p1[0].y - 0.5;

	int k = 0;

	//CarModel Car = CarModel(roi, binmask, macierzKamery, wspolczynnikiZnieksztalcen, macierzRotacji, wektorTranslacji);
	
	Mat obraz2, obraz3, projection, binprojection, xo;
	Mat czarny_prostokat(roi.height, roi.width, CV_8U, Scalar(0));
	
	while (k != 27)
	{
		/* 
		//manipulacja pozycja i wymiarami prostopadloscianu
		if (k == '1')
			x += .1;
		if (k == 'q')
			x -= .1;
		if (k == '2')
			y += .1;
		if (k == 'w')
			y -= .1;
		if (k == '3')
			z += .1;
		if (k == 'e')
			z -= .1;
		if (k == 'a')
			d += .1;
		if (k == 'z')
			d -= .1;
		if (k == 's')
			s += .1;
		if (k == 'x')
			s -= .1;
		if (k == 'd')
			w += .1;
		if (k == 'c')
			w -= .1;
		if (k == 'r')
			a += .1;
		if (k == 'f')
			a -= .1;
		if (k == 't')
			b += .1;
		if (k == 'g')
			b -= .1;
		if (k == 'v')
			angle += 1;
		if (k == 'b')
			angle -= 1;
		if (angle == 360)
			angle = 0;
		//przewijanie do przodu i do tylu po klatce - nie uzywac w tej wersji
		if (k == '0')
			c >> obraz;
		if (k == '9')
		{
			int pos = c.get(CAP_PROP_POS_FRAMES);
			c.set(CAP_PROP_POS_FRAMES, pos - 2);
			c >> obraz;
		}
		
		obraz2 = obraz.clone();
		obraz3 = obraz_org.clone();

		//#########################################################################################
		//OPERACJE LICZENIA PODOBIENSTWA
		//#########################################################################################
		// wstepne liczenie dla punktu startowego
		// wektor punktow prostopadloscianu
		vector<Point3f> p1_mod;
		vector<Point2f> p2_mod, hull, p2_optymalny;
		Point3f c;
		Scalar suma;
		vector<Point> polygon;
		int cost, cost_p, io, koszt_optymalny = 50000;
		int iteracje = 0;
		float fo;
		Mat obraz_temp;		
		//###################################################################################################
		// optymalizacja parametrow GS
		// sprawdzana jest wartosc funkcji kosztu dla kazdego parametru oddzielnie w calym zakresie zmiennosci, po kolei
		// powtarzane kilka razy
		//a: 0.7 - 1
		//b: 0.7 - 1.2
		//x,y: +- 0.5
		//s: 1.7 - 2.2
		//w: 1.5 - 2.5
		//d: 4 - 7
		int angle_start_1 = rand() % 360;

		//pomiar czasu
		auto czas_poczatkowy = chrono::steady_clock::now();
		// kilka randomowych punktow poczatkowych

			for (int m = 0; m < 3; m++) {

				cost = 50000;
				cout << "Cost" << endl;
				int angle_start = angle_start_1 + m * 120;
				float x_start = xs + ((rand() % 20) - 10) / 20;
				float y_start = ys + ((rand() % 20) - 10) / 20;
				//angle = angle_start;
				x = x_start;
				y = y_start;


				do {
					//opt angle : w zakrese obrotu pelnego
					io = angle;

					//Liczenie funkcji celu dla wszystkich punktow w zakresie zmiennosci

#pragma omp parallel for nowait
					for (int j = 0; j < size(angle_opt); j++) {
						// operacje na punktach
						int angle_ = angle_start + j * 5;
						vector<Point3f> p1_mod_;
						vector<Point2f> p2_mod_, hull_;
						p1_mod_ = init3DModel(x, y, z, s, d, w, a, b, angle_);
						projectPoints(p1_mod_, macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen, p2_mod_);
						convexHull(p2_mod_, hull_, 1, 1);
						vector<Point> polygon_ = {};
						for (int i = 0; i < hull_.size(); i++) {
							polygon_.push_back(Point(int(hull_[i].x), int(hull_[i].y)));
						}
						//operacje na obrazach
						Mat czarny_prostokat_(roi.height, roi.width, CV_8U, Scalar(0));
						Mat binprojection_, xo_;
						drawContours(czarny_prostokat_, vector<vector<Point>>{polygon_}, -1, Scalar(255), FILLED, 8, noArray(), 2147483647, Point(-roi.x, -roi.y));
						threshold(czarny_prostokat_, binprojection_, 254, 255, THRESH_BINARY);
						bitwise_xor(binmask, binprojection_, xo_);
						angle_opt[j] = sum(xo_)[0] / 255;
					}
					
					// szukanie indeksu min
					int temp = angle_opt[0];
					int l_opt = 100;
					for (int l = 0; l < size(angle_opt) - 1; l++) {
						if (angle_opt[l + 1] < temp) {
							l_opt = l + 1;
							temp = angle_opt[l + 1];
						}
					}

					//Ustawienie parametru na wartosc optymalna
					if (l_opt != 100) angle = (angle_start + l_opt * 5);
					else angle = io;

					// opt x : w zakresie +-0.5 od poprzedniego x
					fo = x;
					x_start = x - 0.5;
					
#pragma omp parallel for nowait
					for (int j = 0; j < size(x_opt); j++) {
						float x_ = x_start + j * 0.1;
						vector<Point3f> p1_mod_;
						vector<Point2f> p2_mod_, hull_;
						p1_mod_ = init3DModel(x_, y, z, s, d, w, a, b, angle);
						projectPoints(p1_mod_, macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen, p2_mod_);
						convexHull(p2_mod_, hull_, 1, 1);
						vector<Point> polygon_ = {};
						for (int i = 0; i < hull_.size(); i++) {
							polygon_.push_back(Point(int(hull_[i].x), int(hull_[i].y)));
						}
						//operacje na obrazach
						Mat czarny_prostokat_(roi.height, roi.width, CV_8U, Scalar(0));
						Mat binprojection_, xo_;
						drawContours(czarny_prostokat_, vector<vector<Point>>{polygon_}, -1, Scalar(255), FILLED, 8, noArray(), 2147483647, Point(-roi.x, -roi.y));
						threshold(czarny_prostokat_, binprojection_, 254, 255, THRESH_BINARY);
						bitwise_xor(binmask, binprojection_, xo_);
						x_opt[j] = sum(xo_)[0] / 255;
					}

					// szukanie indeksu min
					l_opt = 100;
					float tempf = x_opt[0];
					for (int l = 0; l < size(x_opt) - 1; l++) {
						if (x_opt[l + 1] < tempf) {
							l_opt = l + 1;
							tempf = x_opt[l + 1];
						}
					}
					if (l_opt != 100) x = x_start + l_opt * 0.1;
					else x = fo;

					//opt y : w zakresie +-0.5 od poprzedniego y
					fo = y;
					y_start = y - 0.5;

#pragma omp parallel for nowait				
					for (int j = 0; j < size(y_opt); j++) {
						float y_ = y_start + j * 0.1;
						vector<Point3f> p1_mod_;
						vector<Point2f> p2_mod_, hull_;
						p1_mod_ = init3DModel(x, y_, z, s, d, w, a, b, angle);
						projectPoints(p1_mod_, macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen, p2_mod_);
						convexHull(p2_mod_, hull_, 1, 1);
						vector<Point> polygon_ = {};
						for (int i = 0; i < hull_.size(); i++) {
							polygon_.push_back(Point(int(hull_[i].x), int(hull_[i].y)));
						}
						//operacje na obrazach
						Mat czarny_prostokat_(roi.height, roi.width, CV_8U, Scalar(0));
						Mat binprojection_, xo_;
						drawContours(czarny_prostokat_, vector<vector<Point>>{polygon_}, -1, Scalar(255), FILLED, 8, noArray(), 2147483647, Point(-roi.x, -roi.y));
						threshold(czarny_prostokat_, binprojection_, 254, 255, THRESH_BINARY);
						bitwise_xor(binmask, binprojection_, xo_);
						y_opt[j] = sum(xo_)[0] / 255;
					}

					// szukanie indeksu min
					l_opt = 100;
					tempf = y_opt[0];
					for (int l = 0; l < size(y_opt) - 1; l++) {
						if (y_opt[l + 1] < tempf) {
							l_opt = l + 1;
							tempf = y_opt[l + 1];
						}
					}
					if (l_opt != 100) y = y_start + l_opt * 0.1;
					else y = fo;

					//opt d : w stalym zakresie
					fo = d;

#pragma omp parallel for nowait
					for (int j = 0; j < size(d_opt); j++) {
						float d_ = d_start + j * 0.2;
						vector<Point3f> p1_mod_;
						vector<Point2f> p2_mod_, hull_;
						p1_mod_ = init3DModel(x, y, z, s, d_, w, a, b, angle);
						projectPoints(p1_mod_, macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen, p2_mod_);
						convexHull(p2_mod_, hull_, 1, 1);
						vector<Point> polygon_ = {};
						for (int i = 0; i < hull_.size(); i++) {
							polygon_.push_back(Point(int(hull_[i].x), int(hull_[i].y)));
						}
						//operacje na obrazach
						Mat czarny_prostokat_(roi.height, roi.width, CV_8U, Scalar(0));
						Mat binprojection_, xo_;
						drawContours(czarny_prostokat_, vector<vector<Point>>{polygon_}, -1, Scalar(255), FILLED, 8, noArray(), 2147483647, Point(-roi.x, -roi.y));
						threshold(czarny_prostokat_, binprojection_, 254, 255, THRESH_BINARY);
						bitwise_xor(binmask, binprojection_, xo_);
						d_opt[j] = sum(xo_)[0] / 255;
					}

					// szukanie indeksu min
					l_opt = 100;
					tempf = d_opt[0];
					for (int l = 0; l < size(d_opt) - 1; l++) {
						if (d_opt[l + 1] < tempf) {
							l_opt = l + 1;
							tempf = d_opt[l + 1];
						}
					}
					if (l_opt != 100) d = d_start + l_opt * 0.1;
					else d = fo;

					//opt w : w stalym zakresie
					fo = w;

#pragma omp parallel for nowait
					for (int j = 0; j < size(w_opt); j++) {
						float w_ = w_start + j * 0.1;
						vector<Point3f> p1_mod_;
						vector<Point2f> p2_mod_, hull_;
						p1_mod_ = init3DModel(x, y, z, s, d, w_, a, b, angle);
						projectPoints(p1_mod_, macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen, p2_mod_);
						convexHull(p2_mod_, hull_, 1, 1);
						vector<Point> polygon_ = {};
						for (int i = 0; i < hull_.size(); i++) {
							polygon_.push_back(Point(int(hull_[i].x), int(hull_[i].y)));
						}
						//operacje na obrazach
						Mat czarny_prostokat_(roi.height, roi.width, CV_8U, Scalar(0));
						Mat binprojection_, xo_;
						drawContours(czarny_prostokat_, vector<vector<Point>>{polygon_}, -1, Scalar(255), FILLED, 8, noArray(), 2147483647, Point(-roi.x, -roi.y));
						threshold(czarny_prostokat_, binprojection_, 254, 255, THRESH_BINARY);
						bitwise_xor(binmask, binprojection_, xo_);
						w_opt[j] = sum(xo_)[0] / 255;
					}

					// szukanie indeksu min
					l_opt = 100;
					tempf = w_opt[0];
					for (int l = 0; l < size(w_opt) - 1; l++) {
						if (w_opt[l + 1] < tempf) {
							l_opt = l + 1;
							tempf = w_opt[l + 1];
						}
					}
					if (l_opt != 100) w = w_start + l_opt * 0.1;
					else w = fo;

					//opt s : w stalym zakresie
					fo = s;

#pragma omp parallel for nowait
					for (int j = 0; j < size(s_opt); j++) {
						float s_ = s_start + j * 0.1;
						vector<Point3f> p1_mod_;
						vector<Point2f> p2_mod_, hull_;
						p1_mod_ = init3DModel(x, y, z, s_, d, w, a, b, angle);
						projectPoints(p1_mod_, macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen, p2_mod_);
						convexHull(p2_mod_, hull_, 1, 1);
						vector<Point> polygon_ = {};
						for (int i = 0; i < hull_.size(); i++) {
							polygon_.push_back(Point(int(hull_[i].x), int(hull_[i].y)));
						}
						//operacje na obrazach
						Mat czarny_prostokat_(roi.height, roi.width, CV_8U, Scalar(0));
						Mat binprojection_, xo_;
						drawContours(czarny_prostokat_, vector<vector<Point>>{polygon_}, -1, Scalar(255), FILLED, 8, noArray(), 2147483647, Point(-roi.x, -roi.y));
						threshold(czarny_prostokat_, binprojection_, 254, 255, THRESH_BINARY);
						bitwise_xor(binmask, binprojection_, xo_);
						s_opt[j] = sum(xo_)[0] / 255;
					}

					// szukanie indeksu min
					l_opt = 100;
					tempf = s_opt[0];
					for (int l = 0; l < size(s_opt) - 1; l++) {
						if (s_opt[l + 1] < tempf) {
							l_opt = l + 1;
							tempf = s_opt[l + 1];
						}
					}
					if (l_opt != 100) s = s_start + l_opt * 0.1;
					else s = fo;

					//opt a : w stalym zakresie
					fo = a;

#pragma omp parallel for nowait
					for (int j = 0; j < size(a_opt); j++) {
						float a_ = a_start + j * 0.1;
						vector<Point3f> p1_mod_;
						vector<Point2f> p2_mod_, hull_;
						p1_mod_ = init3DModel(x, y, z, s, d, w, a_, b, angle);
						projectPoints(p1_mod_, macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen, p2_mod_);
						convexHull(p2_mod_, hull_, 1, 1);
						vector<Point> polygon_ = {};
						for (int i = 0; i < hull_.size(); i++) {
							polygon_.push_back(Point(int(hull_[i].x), int(hull_[i].y)));
						}
						//operacje na obrazach
						Mat czarny_prostokat_(roi.height, roi.width, CV_8U, Scalar(0));
						Mat binprojection_, xo_;
						drawContours(czarny_prostokat_, vector<vector<Point>>{polygon_}, -1, Scalar(255), FILLED, 8, noArray(), 2147483647, Point(-roi.x, -roi.y));
						threshold(czarny_prostokat_, binprojection_, 254, 255, THRESH_BINARY);
						bitwise_xor(binmask, binprojection_, xo_);
						a_opt[j] = sum(xo_)[0] / 255;
					}

					// szukanie indeksu min
					l_opt = 100;
					tempf = a_opt[0];
					for (int l = 0; l < size(a_opt) - 1; l++) {
						if (a_opt[l + 1] < tempf) {
							l_opt = l + 1;
							tempf = a_opt[l + 1];
						}
					}
					if (l_opt != 100) a = a_start + l_opt * 0.1;
					else a = fo;

					//opt b : w stalym zakresie
					fo = b;

#pragma omp parallel for nowait
					for (int j = 0; j < size(b_opt); j++) {
						float b_ = b_start + j * 0.1;
						vector<Point3f> p1_mod_;
						vector<Point2f> p2_mod_, hull_;
						p1_mod_ = init3DModel(x, y, z, s, d, w, a, b_, angle);
						projectPoints(p1_mod_, macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen, p2_mod_);
						convexHull(p2_mod_, hull_, 1, 1);
						vector<Point> polygon_ = {};
						for (int i = 0; i < hull_.size(); i++) {
							polygon_.push_back(Point(int(hull_[i].x), int(hull_[i].y)));
						}
						//operacje na obrazach
						Mat czarny_prostokat_(roi.height, roi.width, CV_8U, Scalar(0));
						Mat binprojection_, xo_;
						drawContours(czarny_prostokat_, vector<vector<Point>>{polygon_}, -1, Scalar(255), FILLED, 8, noArray(), 2147483647, Point(-roi.x, -roi.y));
						threshold(czarny_prostokat_, binprojection_, 254, 255, THRESH_BINARY);
						bitwise_xor(binmask, binprojection_, xo_);
						b_opt[j] = sum(xo_)[0] / 255;
					}

					// szukanie indeksu min
					l_opt = 100;
					tempf = b_opt[0];
					for (int l = 0; l < size(b_opt) - 1; l++) {
						if (b_opt[l + 1] < tempf) {
							l_opt = l + 1;
							tempf = b_opt[l + 1];
						}
					}
					if (l_opt != 100) b = b_start + l_opt * 0.1;
					else b = fo;

					//#############################################################################################
					// rysowanie jeszcze raz dla zoptymalizowanych parametrow
					vector<Point3f> p1_mod_;
					vector<Point2f> hull_;
					p1_mod_ = init3DModel(x, y, z, s, d, w, a, b, angle);
					projectPoints(p1_mod_, macierzRotacji, wektorTranslacji, macierzKamery, wspolczynnikiZnieksztalcen, p2_mod);
					convexHull(p2_mod, hull_, 1, 1);
					vector<Point> polygon_ = {};
					for (int i = 0; i < hull_.size(); i++) {
						polygon_.push_back(Point(int(hull_[i].x), int(hull_[i].y)));
					}
					//operacje na obrazach
					Mat czarny_prostokat_(roi.height, roi.width, CV_8U, Scalar(0));
					Mat binprojection_, xo_;
					drawContours(czarny_prostokat_, vector<vector<Point>>{polygon_}, -1, Scalar(255), FILLED, 8, noArray(), 2147483647, Point(-roi.x, -roi.y));
					threshold(czarny_prostokat_, binprojection_, 254, 255, THRESH_BINARY);
					bitwise_xor(binmask, binprojection_, xo_);
					cost_p = cost;
					cost = sum(xo_)[0] / 255;
					iteracje++;
					cout << cost << endl;
					// wyswietlanie
					obraz_temp = obraz2.clone();
					rysuj(obraz_temp, p2_mod);
					//imshow("obraz", obraz_temp);
					//imshow("binmask", binmask);
					//imshow("roi", projection);
					//cvtColor(xo, xo, COLOR_GRAY2BGR);
					//putText(xo, to_string(diff_pixels), Point(0, xo.rows - 5), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(255, 0, 0), 2);
					//imshow("xor", xo);
					//waitKey(1);

				} while (abs(cost_p - cost) > 5 && iteracje <= 15);

				//cout << cost << endl;
				cout << "Iteracje" << endl;
				cout << iteracje << endl;
				cout << "---------------------" << endl;
				iteracje = 0;

				if (cost < koszt_optymalny) {
					koszt_optymalny = cost;
					p2_optymalny = p2_mod;
				}
			}
		
		auto czas_koncowy = chrono::steady_clock::now();
		cout << "Czas optymalizacji: " << chrono::duration_cast<chrono::milliseconds>(czas_koncowy - czas_poczatkowy).count() << "ms" << endl;

		//rysowanie dla optymalnych parametrow koncowych
		obraz3 = obraz_org.clone();
		convexHull(p2_optymalny, hull, 1, 1);
		vector<Point> polygon1;
		for (int i = 0; i < hull.size(); i++) {
			polygon1.push_back(Point(int(hull[i].x), int(hull[i].y)));
		}
		drawContours(obraz3, vector<vector<Point>>{polygon1}, -1, Scalar(255, 255, 255), FILLED);
		projection = obraz3(roi);
		cvtColor(projection, projection, COLOR_BGR2GRAY);
		threshold(projection, binprojection, 254, 255, THRESH_BINARY);
		bitwise_xor(binmask, binprojection, xo);

		// wyswietlanie
		obraz_temp = obraz2.clone();
		rysuj(obraz_temp, p2_optymalny);
		imshow("obraz", obraz_temp);
		imshow("binmask", binmask);
		imshow("roi", projection);
		cvtColor(xo, xo, COLOR_GRAY2BGR);
		//putText(xo, to_string(int(sum(xo)[0] / 255)), Point(0, xo.rows - 5), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(255, 0, 0), 2);
		imshow("xor", xo);
		k = waitKey();
	}
	return 0;
}
*/

/*
void rysuj(Mat& obraz2, vector<Point2f> p2_mod) {
	line(obraz2, p2_mod[0], p2_mod[1], CV_RGB(255, 0, 0), 2);
	line(obraz2, p2_mod[1], p2_mod[2], CV_RGB(255, 0, 0), 2);
	line(obraz2, p2_mod[2], p2_mod[3], CV_RGB(255, 0, 0), 2);
	line(obraz2, p2_mod[3], p2_mod[0], CV_RGB(255, 0, 0), 2);

	line(obraz2, p2_mod[4], p2_mod[8], CV_RGB(255, 255, 0), 2);
	line(obraz2, p2_mod[8], p2_mod[9], CV_RGB(255, 255, 0), 2);
	line(obraz2, p2_mod[9], p2_mod[7], CV_RGB(255, 255, 0), 2);
	line(obraz2, p2_mod[7], p2_mod[4], CV_RGB(255, 255, 0), 2);

	line(obraz2, p2_mod[0], p2_mod[4], CV_RGB(255, 0, 255), 2);
	line(obraz2, p2_mod[3], p2_mod[7], CV_RGB(255, 0, 255), 2);

	line(obraz2, p2_mod[1], p2_mod[5], CV_RGB(0, 255, 0), 2);
	line(obraz2, p2_mod[2], p2_mod[6], CV_RGB(0, 255, 0), 2);

	line(obraz2, p2_mod[5], p2_mod[8], CV_RGB(0, 0, 255), 2);
	line(obraz2, p2_mod[6], p2_mod[9], CV_RGB(0, 0, 255), 2);
	line(obraz2, p2_mod[5], p2_mod[6], CV_RGB(0, 0, 255), 2);
}
*/
/*
//Rotacja punktu wokol osi z, umieszczonej w punkcie [center.x , center.y]
Point3f rotateOverZ(Point3f point, Point3f center, int degrees) {
	float x = point.x;
	float y = point.y;
	float z = point.z;
	float rad = degrees * 3.14 / 180;
	Mat R = (Mat_<float>(3, 3) << cos(rad), -sin(rad), 0,
									sin(rad), cos(rad), 0,
									0,			0,		1);
	Point3f result{ x, y, z };
	Mat v = (Mat_<float>(3, 1) << result.x - center.x, result.y - center.y, result.z);
	v = R * v;
	result.x = v.at<float>(0, 0) + center.x;
	result.y = v.at<float>(1, 0) + center.y;
	result.z = v.at<float>(2, 0);
	return result;
}*/
/*
vector<Point3f> init3DModel(float x, float y, float z, float s, float d, float w, float a, float b, int angle) {
	vector<Point3f> p1_mod = { { x, y, z }, { x + d, y, z }, { x + d, y + s, z }, { x, y + s, z },
									 { x, y, z + w }, { x + d, y, z + w - a }, { x + d, y + s, z + w - a }, { x, y + s, z + w },
									 { x + d - b, y, z + w }, { x + d - b, y + s, z + w } };
	Point3f c = { x + d / 2, y + s / 2, 0 };
	for (int i = 0; i < p1_mod.size(); i++) {
		p1_mod[i] = rotateOverZ(p1_mod[i], c, angle);
	}
	return p1_mod;
}*/

	/*
	// liczenie wysokosci maski w przyblizeniu (w polowie maski) -> w pikselach
	Mat_<int> tmp(1,szer);
	reduce(mask, tmp, 0, REDUCE_SUM, CV_32S);
	int car_h = tmp(0,int(szer/2));
	tmp.release();

	// liczenie szerokosci maski w przyblizeniu (w polowie maski) -> w pikselach
	Mat_<int> tmp2(wys, 1);
	reduce(mask, tmp2, 1, REDUCE_SUM, CV_32S);
	int car_w = tmp2(int(wys / 2), 0);
	tmp2.release();
	*/


