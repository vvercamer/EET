#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <complex>
#include <gsl/gsl_statistics_double.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_fit.h>
typedef std::complex<double> complex_d;

#include "toto.h"
#include "gnuplot_i.h"


int main(int argc, char *argv[])
{
	double deltaT = 0.1;
	int nbsamples = 10000;
	int nbosc = 100;
	int nbrand = 5;
	int nbK = 10;

	if (argc == 6) {
		deltaT = strtod(argv[1],NULL);
		nbsamples = (int)strtol(argv[2], NULL, 10);
		nbosc = (int)strtol(argv[3], NULL, 10);
		nbrand = (int)strtol(argv[4], NULL, 10);
		nbK = (int)strtol(argv[5], NULL, 10);
	}

	else if (argc == 1) {
	}
	else if (argc == 2) {
		if (strcmp(argv[1],"-h\n")) {
			printf("utilisation :\n ./toto deltaT nbsamples nbosc nbrand nbK\n");
		}
		return 0;
	}
	else {
		printf("problème sur les paramètres d'entrée\n");
		return 1;
	}

	/*définition des conditions expérimentales*/
	double *temps = (double *) malloc (nbsamples*sizeof(double));
	srand(time(NULL));

	/*définition des oscillatteurs*/
	int idxOsc, idxTime, idxK, idxRand;
	
	double rayontemp = 0, psitemp = 0;
	
	double *omega = (double *) malloc (nbosc*sizeof(double));
	double *theta = (double *) malloc (nbosc*sizeof(double));
	double K = 0;
	double OMEGA = 0;
	double sigma = 0.1;
//	double subcrit = 0.2;
	double *rayon = (double *) malloc (nbsamples*sizeof(double));
	double *psi = (double *) malloc (nbsamples*sizeof(double));
	double *rayonmoyen = (double *) malloc (nbsamples*sizeof(double));
	double *rmoy = (double *) malloc (nbsamples*sizeof(double));		/*moyenne sur es réalisations*/

	double *rayonstable = (double *) malloc (nbK*sizeof(double));
	double *invrayonstable = (double *) malloc (nbK*sizeof(double));
	double *Kvect = (double *) malloc (nbK*sizeof(double));

//	double *ecartMax = (double *) malloc (nbosc*sizeof(double));
	double ecartMax = 0;

	/*Définition pour les logarithmes*/
	int idxKc = 0;
	int idxprim = 0;
	double *logk = (double *) malloc ((nbK-idxKc)*sizeof(double));
	double *logr = (double *) malloc ((nbK-idxKc)*sizeof(double));
	double *deltarstable = (double *) malloc ((nbK-idxKc)*sizeof(double));
	double *KvectCut = (double *) malloc ((nbK-idxKc)*sizeof(double));
	double *TcCut = (double *) malloc ((nbK-idxKc)*sizeof(double));
		
	/*Détermination de la distribution des pulsations propres*/
	int nbw = 1001;
	int idxw;
	double *w = (double *) malloc (nbw*sizeof(double));
	double *N = (double *) malloc (nbw*sizeof(double));

	/*Détermination de Kc*/
	double Kc;
	Kc = 2 / (M_PI * gsl_ran_cauchy_pdf(0, sigma));
	printf("Kc = %f\n",Kc);

	/*Déclaration des variables nécessaires à la détermination du rayon asymptotique rayonInfini*/
	int IdxC;
	double rMax = 0;
	double *Tc = (double *) malloc (nbK*sizeof(double));
	double *rayonCut = (double *) malloc (nbsamples*sizeof(double));
	double *rayonInfini = (double *) malloc (nbK*sizeof(double));


	/*Déclaration des variables nécessaires pour Runge-Kutta 4*/

	double k1, k2, k3, k4;



	/*Boucle sur les valeurs de K*/
	for(idxK = 0 ; idxK < nbK ; idxK++) {
		if  (nbK == 1) {
			printf("entrer la valeur de K\n");
			scanf("%lf",&K);
		}
		else {
			K = (double)idxK * 2 / (nbK - 1);
		}

		printf("K = %f\n",K);

		rayonstable[idxK] = 0;
       		int idxTimeStart;
		idxTimeStart = (int)floor(3 * nbsamples / 4.);


		for (idxTime=1 ; idxTime < nbsamples ; idxTime++) {
			rmoy[idxTime] = 0;
		}


		/*Boucle sur les réalisations*/
		for (idxRand = 0 ; idxRand < nbrand ; idxRand++) {
			/*Détermination des oscillations propres des oscillateurs*/
			const gsl_rng_type * randType;
			gsl_rng * r;
			gsl_rng_env_setup();
			randType = gsl_rng_default;
			r = gsl_rng_alloc (randType);
//			*r = (*r) + (gsl_rng)(1);


			for(idxOsc = 0 ; idxOsc < nbosc ; idxOsc++) {
//				omega[idxOsc] = OMEGA + gsl_ran_gaussian(r,sigma);
				omega[idxOsc] = OMEGA + gsl_ran_cauchy(r,sigma);
//				omega[idxOsc] = OMEGA + (gsl_ran_cauchy(r, sigma) - subcrit + gsl_ran_cauchy(r, sigma) + subcrit) / 2;
			}


			/*Initialisation de theta, de rayon et de psi*/
			for(idxOsc = 0 ; idxOsc < nbosc ; idxOsc++) {
				theta[idxOsc] = fmod( rand(), 2*M_PI)-M_PI;
			}

			for(idxTime = 0 ; idxTime < nbsamples ; idxTime++) {
				rayon[idxTime]=0;
				psi[idxTime]=0;
			}

			meanField(theta , &rayontemp, &psitemp, nbosc);
			rayon[0] = rayontemp;
			psi[0] = psitemp;


			/*Corps du programme*/

			for (idxTime = 1 ; idxTime < nbsamples ; idxTime++) {
		      	temps[idxTime] = idxTime*deltaT;

				for (idxOsc = 0 ; idxOsc < nbosc ; idxOsc++) {
					/*Méthode de runge-kutta d'ordre 4*/
					k1 = deltaT * kuramoto(omega[idxOsc], K, psi[idxTime-1], rayon[idxTime-1], theta[idxOsc]);
					k2 = deltaT * kuramoto(omega[idxOsc], K, psi[idxTime-1], rayon[idxTime-1], theta[idxOsc] + deltaT * k1 / 2.0);
					k3 = deltaT * kuramoto(omega[idxOsc], K, psi[idxTime-1], rayon[idxTime-1], theta[idxOsc] + deltaT * k2 / 2.0);
					k4 = deltaT * kuramoto(omega[idxOsc], K, psi[idxTime-1], rayon[idxTime-1], theta[idxOsc] + deltaT * k3);
					theta[idxOsc] = theta[idxOsc] + (k1 + 2 * k2 + 2 * k3 + k4) * deltaT / 6.0;
				}

				meanField(theta , &rayontemp, &psitemp, nbosc);
			   	rayon[idxTime]=rayontemp;
			psi[idxTime]=fmod(psitemp, 2 * M_PI);

				if (idxTime > idxTimeStart)
					rayonstable[idxK] += rayontemp;

				rmoy[idxTime]+=rayontemp;
			}


		}

		/*Fin de la boucle sur les réalisations*/

		/*Détermination de rmoy, moyenne de r sur plusieurs réalisations*/
		for (idxTime = 1 ; idxTime < nbsamples ; idxTime++) {
			rmoy[idxTime]/=nbrand;
		}

		/*Détermination de rayonstable et de son inverse*/
		rayonstable[idxK] /= (nbsamples - idxTimeStart + 1) * nbrand;
		invrayonstable[idxK] = 1 / (1 - rayonstable[idxK] * rayonstable[idxK]);//pour formule 4.7

		/*Détermination du vecteur Kvect*/
		Kvect[idxK] = K;


		/*Définition de rMax*/
		for (idxTime = 1 ; idxTime < nbsamples ; idxTime++) {
			if (rmoy[idxTime] > rMax)
				rMax = rmoy[idxTime];
		}


		/*Définition du temps critique comme étant le temps pour lequel on atteint 90% de la valeur maximale*/
		idxTime = 0;
		while (rmoy[idxTime] < 0.9 * rMax) {
			idxTime++;
		}
		IdxC = idxTime;
		Tc[idxK] = IdxC*deltaT;

		/*Détermination de rayonInfini*/
		if (IdxC < nbsamples) {
			for (idxTime = IdxC; idxTime<nbsamples; idxTime++) {
				rayonCut[idxTime-IdxC] = rmoy[idxTime];
			}
			rayonInfini[idxK] = gsl_stats_mean(rayonCut, 1, nbsamples-IdxC+1);
//			printf("%g pour K = %d \n",rayonInfini[idxK], idxK);
		}
		else {
//			printf("%g pour K= %d \n", rayonInfini[idxK], idxK);
			rayonInfini[idxK] = gsl_stats_mean(rmoy, 1, nbsamples);
/*car sinon on obtient des valeurs nulles du rayonInfini pour certaines valeurs de K<Kc*/
		}


		if (nbK == 1) {
			rayonmoyen[0] = rayon[0];
			for (idxTime = 1 ; idxTime < nbsamples ; idxTime++) {
				rayonmoyen[idxTime] = rayonmoyen[idxTime-1] + rayon[idxTime];
			}
			for (idxTime = 0 ; idxTime < nbsamples ; idxTime++) {
				rayonmoyen[idxTime] /= idxTime+1;
			}
		}
	}


	/*Détermination des logarithmes*/
	while (Kvect[idxKc] <= Kc) {
		idxKc += 1;
	}
	idxKc += 4;

	for (idxK = idxKc ; idxK < nbK ; idxK++) {
		idxprim = idxK - idxKc;
		logk[idxprim] = log(1 - Kc / Kvect[idxK]);
		logr[idxprim] = log(rayonstable[idxK]);
		deltarstable[idxprim] = sqrt(1 - Kc / Kvect[idxK]) - rayonstable[idxK];
		if (deltarstable[idxK] > ecartMax)
			ecartMax = deltarstable[idxK];
		KvectCut[idxprim] = Kvect[idxK];
		TcCut[idxprim] = Tc[idxK];
	}



	/*Régression linéaire*/
	double c0, c1, cov00, cov01, cov11, sumsq;  
        size_t xstride = 1;
	size_t ystride = 1;
	size_t n = nbK - idxKc;
	double *fit = (double *) malloc (n*sizeof(double));
	gsl_fit_linear(logk, xstride, logr, ystride, n, &c0, &c1, &cov00, &cov01, &cov11, &sumsq);
	for (idxK=0 ; idxK < nbK-idxKc ; idxK++) {
		fit[idxK] = c0 + c1 * logk[idxK];
	}



	/*Détermination de la distribution des pulsations propres*/
	for (idxw=0 ; idxw < nbw ; idxw++) {
		w[idxw] = (-(nbw - 1) / 2 + idxw) * 0.8 * 2 / nbw;
//		N[idxw] = gsl_ran_cauchy_pdf(w[idxw], sigma)-1;
		N[idxw] = gsl_ran_cauchy_pdf(w[idxw] - 0.2, sigma) / 2 + gsl_ran_cauchy_pdf(w[idxw] + 0.2, sigma) / 2;
	}
	
	

	/*Détermination du vecteur "numéro de l'oscillateur"*/
	double *Nosc = (double *) malloc (nbosc*sizeof(double));

	for(idxOsc = 0 ; idxOsc < nbosc ; idxOsc++) {
		Nosc[idxOsc]=idxOsc;
	}



	/*Définitions pour le graphique*/
	gnuplot_ctrl * gp;
	gp = gnuplot_init();

	char titre[256];
#if defined ( __APPLE__ )
	gnuplot_cmd(gp, "set terminal x11 0 persist");
#else
	gnuplot_cmd(gp, "set terminal wxt 0 persist");
#endif
	gnuplot_setstyle(gp, "lines");
	gnuplot_set_ylabel(gp, "distribution g(w)");
	gnuplot_set_xlabel(gp, "pulsation w");
	gnuplot_plot_xy(gp, w, N, nbw,"distribution des pulsations propres des oscillateurs");


	gp = gnuplot_init();
#if defined ( __APPLE__ )
	gnuplot_cmd(gp, "set terminal x11 1 persist");
#else
	gnuplot_cmd(gp, "set terminal wxt 1 persist");
#endif
	gnuplot_setstyle(gp, "linespoints");
	gnuplot_set_ylabel(gp, "r");

	if (nbK == 1) {
		gnuplot_setstyle(gp, "dots");
		gnuplot_set_xlabel(gp, "t");
		gnuplot_cmd(gp, "set yrange [-0.05:1.05]");
		sprintf(titre,"evolution de r(t) pour K = %f", K);
		gnuplot_plot_xy(gp, temps, rayon, nbsamples, titre);
		sprintf(titre,"evolution de rmoyen(t) pour K = %f", K);
		gnuplot_plot_xy(gp, temps, rayonmoyen, nbsamples, titre) ;
		sprintf(titre,"evolution de rmoy(t) pour K = %f", K);
		gnuplot_plot_xy(gp, temps, rmoy, nbsamples, titre);
		
		gp = gnuplot_init();
#if defined ( __APPLE__ )
		gnuplot_cmd(gp, "set terminal x11 2 persist");
#else
		gnuplot_cmd(gp, "set terminal wxt 2 persist");
#endif
		gnuplot_setstyle(gp, "lines");
		gnuplot_set_ylabel(gp, "theta");
		gnuplot_set_xlabel(gp, "numéro de l'oscillateur");
		gnuplot_plot_xy(gp, Nosc, omega, nbosc,"angles des oscillateurs");
	}

	else {
		gnuplot_set_xlabel(gp, "K");
   		gnuplot_cmd(gp, "set yrange [-0.05:1.05]");
		gnuplot_plot_xy(gp, Kvect, rayonstable, nbK, "evolution de rstable(K)");
		gnuplot_plot_xy(gp, Kvect, rayonInfini, nbK, "evolution de rinfini(K)");

		gp = gnuplot_init();
#if defined ( __APPLE__ )
		gnuplot_cmd(gp, "set terminal x11 2 persist");
#else
		gnuplot_cmd(gp, "set terminal wxt 2 persist");
#endif
		gnuplot_setstyle(gp, "linespoints");
		gnuplot_set_ylabel(gp, "log rstable");
		gnuplot_set_xlabel(gp, "log (K-Kc)/K");
//		gnuplot_cmd(gp, "set yrange [-0.05:10.05]");
//		gnuplot_plot_xy(gp, Kvect, invrayonstable, nbK, "evolution de rstable en fonction de K");
		sprintf(titre,"evolution de rstable en fonction de (K-Kc)/K en log-log");
		gnuplot_plot_xy(gp, logk, logr, (nbK-idxKc), titre);
		sprintf(titre,"régression linéaire");
		gnuplot_plot_xy(gp, logk, fit, (nbK-idxKc), titre);

	/*Tracé de l'évolution du temps caractéractique en fonction de K*/
		gp = gnuplot_init();
		gnuplot_set_ylabel(gp, "temps caractéristique");
		gnuplot_set_xlabel(gp, "K");

#if defined ( __APPLE__ )
   	 	gnuplot_cmd(gp, "set terminal x11 3 persist");
#else
		gnuplot_cmd(gp, "set terminal wxt 3 persist");
#endif
		gnuplot_setstyle(gp, "linespoints");
		gnuplot_plot_xy(gp, KvectCut, TcCut, (nbK-idxKc), "evolution du temps caracteristique");
	/*il y a un souci au niveau de l'avant dernier point pour nbK=20*/

	/*Tracé de l'évolution de l'écart des r simulés et théoriques*/
		gp = gnuplot_init();
		gnuplot_set_ylabel(gp, "r théorique - r simulé");
		gnuplot_set_xlabel(gp, "K");

#if defined ( __APPLE__ )
	    	gnuplot_cmd(gp, "set terminal x11 4 persist");
#else
		gnuplot_cmd(gp, "set terminal wxt 4 persist");
#endif
		gnuplot_setstyle(gp, "linespoints");
		gnuplot_plot_xy(gp, KvectCut, deltarstable, (nbK-idxKc), "écart entre simulation et théorie");


/*		gp = gnuplot_init();
#if defined ( __APPLE__ )
		gnuplot_cmd(gp, "set terminal x11 5 persist");
#else
		gnuplot_cmd(gp, "set terminal wxt 5 persist");
#endif
		gnuplot_setstyle(gp, "linespoints");
		gnuplot_set_ylabel(gp, "maximum de l'écart entre simulation et théorie");
		gnuplot_set_xlabel(gp, "N");
		gnuplot_plot_xy(gp, Nosc, ecartMax, nbosc, "evolution des fluctuations en fonction du nombre d'oscillateurs") ;*/

	}

	printf("r = %f\npsi = %f\n",rayontemp,psitemp);
	printf("écart entre simulation et théorie = %f\n",ecartMax);
	printf("pente du fit = %f\n",c1);


	/*Libération de la mémoire*/
	free(temps);
	free(omega);
	free(rayon);
	free(psi);
	free(theta);
	free(rmoy);
	free(rayonInfini);
	free(rayonCut);
	gnuplot_close(gp);
	return 0;

}

int meanField(double *theta, double *rayon, double *psi, int nbosc)
{
	int idxOsc;
	complex_d rComplex(0,0);
	for(idxOsc = 0 ; idxOsc < nbosc ; idxOsc++)
	{
		rComplex += exp(complex_d(0,theta[idxOsc]));
	}
	rComplex /= nbosc;

	*rayon = abs(rComplex);
	*psi = arg(rComplex);

	return 0;
}

double kuramoto(double omega, double K, double psi, double rayon, double theta)
{
	return(omega + K * rayon * sin(psi - theta));
}



