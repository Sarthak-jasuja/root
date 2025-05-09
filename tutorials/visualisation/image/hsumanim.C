/// \file
/// \ingroup tutorial_image
/// \notebook
/// This script is a slightly modified version of hsum.C.
///
/// Uncomment the two `c1->Print(...);` lines in order to produce
/// an animated gif file. The option "++" makes an infinite animation.
/// The animated file `hsumanim.gif` can be visualized within a web browser
///
/// \macro_image
/// \macro_code
///
/// \authors Rene Brun, Valeriy Onuchin

void hsumanim()
{
   auto c1 = new TCanvas("c1", "The HSUM example", 200, 10, 600, 400);
   c1->SetGrid();

   gBenchmark->Start("hsum");

   // Create some histograms.
   auto total = new TH1F("total", "This is the total distribution", 100, -4, 4);
   auto main = new TH1F("main", "Main contributor", 100, -4, 4);
   auto s1 = new TH1F("s1", "This is the first signal", 100, -4, 4);
   auto s2 = new TH1F("s2", "This is the second signal", 100, -4, 4);
   total->Sumw2(); // this makes sure that the sum of squares of weights will be stored
   total->SetMarkerStyle(21);
   total->SetMarkerSize(0.7);
   main->SetFillColor(16);
   s1->SetFillColor(42);
   s2->SetFillColor(46);
   TSlider *slider = 0;
   gSystem->Unlink("hsumanim.gif"); // delete old file

   // Fill histograms randomly
   gRandom->SetSeed();
   const Int_t kUPDATE = 500;
   Float_t xs1, xs2, xmain;
   Int_t gifcnt = 0;
   for (Int_t i = 0; i < 10000; i++) {
      xmain = gRandom->Gaus(-1, 1.5);
      xs1 = gRandom->Gaus(-0.5, 0.5);
      xs2 = gRandom->Landau(1, 0.15);
      main->Fill(xmain);
      s1->Fill(xs1, 0.3);
      s2->Fill(xs2, 0.2);
      total->Fill(xmain);
      total->Fill(xs1, 0.3);
      total->Fill(xs2, 0.2);
      if (i && (i % kUPDATE) == 0) {
         if (i == kUPDATE) {
            total->Draw("e1p");
            main->Draw("same");
            s1->Draw("same");
            s2->Draw("same");
            c1->Update();
            slider = new TSlider("slider", "test", 4.2, 0, 4.6, total->GetMaximum(), 38);
            slider->SetFillColor(46);
         }
         if (slider)
            slider->SetRange(0, Float_t(i) / 10000.);
         c1->Modified();
         c1->Update();
         if (gROOT->IsBatch()) {
            // c1->Print("hsumanim.gif+");
            printf("i = %d\n", i);
         } else {
            if (gSystem->ProcessEvents())
               break;
         }
      }
   }
   slider->SetRange(0, 1);
   total->Draw("sameaxis"); // to redraw axis hidden by the fill area
   c1->Modified();
   // c1->Print("hsumanim.gif++")

   gBenchmark->Show("hsum");
}
