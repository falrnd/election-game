﻿#include <Siv3D.hpp>
#include <random>

struct Player {
	int Speech;    // 演説力
	int Kanban;    // 影響力
	int Jiban;     // 協力者
	int Kaban;     // 資金
	int NumSpc; // 演説回数
};

// 状態・日程など
int Situation = 0;
int Turn = 1;
int Life = 0;
int Card = 0, CardID = -1;

// ゲームの状況
bool Hajimete = true;
bool HajimeteEnzetsu = true;
Grid<bool> Choice{ 9, 5 };
Grid<bool> Wakamono{ 9, 5 };
Grid<int> Keisei{ 9, 5 };
Grid<int> NewKeisei{ 9, 5 };
int VoteRate1 = 20;
int VoteRate2 = 60;
int NextAI = 1;
Player Z1, Z2;

// マウスの状況
double WaitTime = 100.00; int IsWaiting = 0;
double ButtonA[100];
double ButtonB[100][100];
double GetLastClick = 0.0;

// 投票確定情報
int Kakutei = 0;
int P1[5][9], P2[5][9];
int FinalVote1 = 0, FinalVote2 = 0, FinalVote3 = 0, TotalVote = 0;
int DisVote1 = 0, DisVote2 = 0;

// 勝率などの履歴
int History[20];
int History2[20][5][9];

// AI の動作確率
double probAI[6][14] = {
	{ 0.40, 0.05, 0.05, 0.10, 0.15, 0.15, 0.15, 0.10, 0.10, 0.72, 0.82, 0.86, 0.86, 0.86 },  // 演説
	{ 0.01, 0.01, 0.23, 0.23, 0.23, 0.50, 0.15, 0.10, 0.10, 0.15, 0.05, 0.01, 0.01, 0.01 },  // 演説を練る
	{ 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.63, 0.73, 0.73, 0.10, 0.10, 0.10, 0.10, 0.10 },  // 資金集め
	{ 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01 },  // 設備購入
	{ 0.17, 0.12, 0.20, 0.50, 0.45, 0.32, 0.05, 0.05, 0.05, 0.01, 0.01, 0.01, 0.01, 0.01 },  // 協力者集め
	{ 0.40, 0.80, 0.50, 0.15, 0.15, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01 }   // ＳＮＳ投稿
};

bool AllZero() {
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			if (Keisei[i][j] != 0) return false;
		}
	}
	return true;
}

int AI_Choice() {
	int Days = (Turn + 2) / 3 - 1, NextChoice = 5;
	if (Z2.Kaban >= 350) {
		NextChoice = 3;
	}
	else if (Turn == 42) {
		if (Z2.Kaban < 20) NextChoice = 2;
		else NextChoice = 0;
	}
	else if (Turn >= 40 && Turn <= 42) {
		if (Z2.Kaban < Z2.NumSpc * 20) NextChoice = 2;
		else NextChoice = 0;
	}
	else {
		while (true) {
			double ret = Random();
			for (int i = 0; i < 6; i++) {
				if (ret < probAI[i][Days]) { NextChoice = i; break; }
				ret -= probAI[i][Days];
			}
			if (NextChoice == 0 && Z2.Kaban < Z2.NumSpc * 20) continue;
			if (NextChoice == 3 && Z2.Kaban < 350) continue;
			if (NextChoice == 0 && Days >= 10 && Days <= 12 && Z2.NumSpc == 1) continue;
			break;
		}
	}
	return NextChoice;
}

void AI_Enzetsu() {
	for (int t = 0; t < Z2.NumSpc; t++) {
		if (Z2.Kaban < 20) break;
		double min_diff = -1.0; int px = -1, py = -1;
		for (int i = 0; i < 5; i++) {
			for (int j = 0; j < 9; j++) {
				double prob1 = 1.0 / (1.0 + exp(0.1 * Keisei[i][j]));
				double prob2 = 1.0 / (1.0 + exp(0.1 * (Keisei[i][j] - Z2.Speech)));
				if (min_diff < prob2 - prob1) {
					min_diff = prob2 - prob1;
					px = i; py = j;
				}
			}
		}
		Keisei[px][py] -= Z2.Speech;
		Z2.Kaban -= 20;
	}
}

double Norm() {
	static std::binomial_distribution dist{ 10000, 0.5 };
	return (dist(s3d::GetDefaultRNG()) - 5000.0) / 50.0;
}

void Vote_Kakutei() {
	double OverAll_Gosa = 0.5 * Norm();
	double Vote_Gosa = 1.5 * Norm();
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			double Ken_Gosa = 1.5 * Norm();
			double VoteRate = 0.01 * (VoteRate1 + Vote_Gosa);
			if (Wakamono[i][j] == false) VoteRate = 0.01 * (VoteRate2 + Vote_Gosa);
			VoteRate += 0.05 * Norm();
			VoteRate = Clamp(VoteRate, 0.0001, 0.9999);
			P1[i][j] = 10000.0 * VoteRate * (1.0 / (1.0 + exp(-(Keisei[i][j] + OverAll_Gosa + Ken_Gosa) / 10.0)));
			P2[i][j] = 10000.0 * VoteRate * (1.0 / (1.0 + exp(+(Keisei[i][j] + OverAll_Gosa + Ken_Gosa) / 10.0)));
		}
	}

	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			if (Wakamono[i][j])
				FinalVote1 += P1[i][j] + P2[i][j];
			else
				FinalVote2 += P1[i][j] + P2[i][j];
			FinalVote3 += P1[i][j] + P2[i][j];
		}
	}
	TotalVote = FinalVote3;
	FinalVote1 /= 15;
	FinalVote2 /= 30;
	FinalVote3 /= 45;
}

void Initialize() {
	Keisei.fill(0);
	Wakamono.fill(false);
	for (int t = 0; t < 15; t++) {
		while (true) {
			int sx = Random(4);
			int sy = Random(8);
			if (Wakamono[sx][sy] == false) { Wakamono[sx][sy] = true; break; }
		}
	}
	Z1.Speech = 5; Z1.Kanban = 0; Z1.Jiban = 10; Z1.Kaban = 100; Z1.NumSpc = 1;
	Z2.Speech = 5; Z2.Kanban = 0; Z2.Jiban = 10; Z2.Kaban = 100; Z2.NumSpc = 1;

	for (int i = 0; i < 15; i++) {
		History[i] = 50;
		for (int j = 0; j < 5; j++) {
			for (int k = 0; k < 9; k++) History2[i][j][k] = 0;
		}
	}
}

void ResetChoice() {
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) { ButtonB[i][j] = 0; Choice[i][j] = false; }
	}
	for (int t = 0; t < 7; t++) {
		while (true) {
			int sx = Random(4);
			int sy = Random(8);
			if (Choice[sx][sy] == false) { Choice[sx][sy] = true; break; }
		}
	}
}

void Apply_SNS() {
	int qx = -1, qy = -1;

	// 自分の影響力
	while (true) {
		int sx = Random(4);
		int sy = Random(8);
		if (Wakamono[sx][sy] == true) { Keisei[sx][sy] += Z1.Kanban; qx = sx; qy = sy; break; }
	}

	// 相手の影響力
	while (true) {
		int sx = Random(4);
		int sy = Random(8);
		if (Wakamono[sx][sy] == true && (sx != qx || sy != qy)) { Keisei[sx][sy] -= Z2.Kanban; break; }
	}

	VoteRate1 += (Z1.Kanban + Z2.Kanban + 5) / 15;
}

void Refresh() {
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) {
			int dx[4] = { 1, 0, -1, 0 };
			int dy[4] = { 0, 1, 0, -1 };

			double score = Keisei[i][j], maxs = Keisei[i][j], mins = Keisei[i][j];
			for (int k = 0; k < 4; k++) {
				int rx = i + dx[k], ry = j + dy[k];
				if (rx < 0 || ry < 0 || rx >= 5 || ry >= 9) continue;
				score -= 0.05 * Keisei[i][j];
				score += 0.10 * Keisei[rx][ry];
				mins = Min(mins, 1.0 * Keisei[rx][ry]);
				maxs = Max(maxs, 1.0 * Keisei[rx][ry]);
			}
			double newscore = Clamp(score, mins, maxs);
			if (newscore >= 0) NewKeisei[i][j] = (int)(0.5 + newscore);
			else NewKeisei[i][j] = -(int)(0.5 - newscore);
		}
	}
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 9; j++) Keisei[i][j] = NewKeisei[i][j];
	}
}

void Main() {
	// 背景を白にする
	Scene::SetBackground(Palette::White);

	// フォントを用意
	const Font font80(80);
	const Font font70(70);
	const Font font60(60);
	const Font font50(50);
	const Font font40(40);
	const Font font35(35);
	const Font font30(30);
	const Font font25(25);
	const Font font20(20);
	const Font font18(18);
	const Font font15(15);
	const Font font10(10);
	const Font font6(6);

	const ColorF black02 = ColorF{ 0.2 };

	const ColorF colorP1 = ColorF{ 0.6, 0.1, 0.3 };
	const ColorF colorP2 = ColorF{ 0.25, 0.25, 0.55 };

	// 初期値を設定
	Initialize();

	while (System::Update()) {
		// -----------------------------------------------------------------------------------------------------------------------
		// ---------------------------------------------- 待ち受け画面 -----------------------------------------------------------
		// -----------------------------------------------------------------------------------------------------------------------
		if (Situation == 0) {
			Rect{ 0, 0, 800, 600 }.draw(colorP2);
			font60(U"Let's Win in the Election").draw(45, 45, Palette::White);
			font30(U"～選挙で勝とう ゲーム版～").draw(205, 150, Palette::White);

			Rect{ 75, 300, 140 }.draw(AlphaF(0.5 + 0.5 * ButtonA[10]));
			Rect{ 245, 300, 140 }.draw(AlphaF(0.5 + 0.5 * ButtonA[11]));
			Rect{ 415, 300, 140 }.draw(AlphaF(0.5 + 0.5 * ButtonA[12]));
			Rect{ 585, 300, 140 }.draw(AlphaF(0.5 + 0.5 * ButtonA[13]));
			font20(U"簡単").draw(125, 335, Palette::Black); font20(U"自分が現職").draw(95, 370, Palette::Black);
			font20(U"普通").draw(295, 335, Palette::Black); font20(U"新人対決").draw(275, 370, Palette::Black);
			font20(U"難しい").draw(455, 335, Palette::Black); font20(U"相手が現職").draw(437, 370, Palette::Black);
			font20(U"とても難しい").draw(595, 335, Palette::Black); font20(U"相手が10選").draw(603, 370, Palette::Black);
			font30(U"クリックしてレベルを選択").draw(220, 500, AlphaF(Periodic::Sine0_1(1.5s)));

			// マウスの状態
			int MouseState = -1;
			for (int i = 0; i < 4; ++i) {
				if (Rect{ 75 + 170 * i, 300, 140 }.mouseOver()) {
					MouseState = i;
					break;
				}
			}

			for (int i = 10; i < 14; i++) {
				if (i != MouseState + 10) ButtonA[i] = Max(0.0, ButtonA[i] - 5.0 * Scene::DeltaTime());
				if (i == MouseState + 10) ButtonA[i] = Min(1.0, ButtonA[i] + 5.0 * Scene::DeltaTime());
			}

			// キーが押された場合
			if (Scene::Time() - GetLastClick >= 0.1 && MouseL.down() && WaitTime >= 1.2) {
				GetLastClick = Scene::Time();
				if (MouseState == 0) { Z1.Jiban = 100; Z2.Jiban = 40; }
				if (MouseState == 1) { Z1.Jiban = 40; Z2.Jiban = 40; }
				if (MouseState == 2) { Z1.Jiban = 40; Z2.Jiban = 100; }
				if (MouseState == 3) { Z1.Jiban = 40; Z2.Jiban = 100; Z2.Kanban = 8; }
				if (MouseState != -1) { WaitTime = 0.0; Situation = 1; }
			}
		}

		// -----------------------------------------------------------------------------------------------------------------------
		// ----------------------------------------------- 選挙運動中 ------------------------------------------------------------
		// -----------------------------------------------------------------------------------------------------------------------
		if (Situation == 1 || Situation == 2 || Situation == 3 || Situation == 4 || Situation == 5 || Situation == 6 || Situation == 8) {
			Rect{ 0, 330, 520, 270 }.draw(ColorF{ 0.90, 0.95, 1.00 });
			Rect{ 520, 330, 280, 270 }.draw(ColorF{ 0.80, 0.90, 1.00 });

			// 表の描画
			font20(U"形勢速報").draw(540, 340, black02);
			Rect{ 560, 385, 200, 196 }.draw(colorP2);
			Rect{ 640, 413, 120, 168 }.draw(Palette::White);
			for (int i = 0; i <= 7; i++) Line{ 560, 385 + 28 * i, 760, 385 + 28 * i }.draw(2, black02);
			Line{ 560, 385, 560, 581 }.draw(2, black02);
			Line{ 640, 385, 640, 581 }.draw(2, black02);
			Line{ 700, 385, 700, 581 }.draw(2, black02);
			Line{ 760, 385, 760, 581 }.draw(2, black02);

			// 形勢評価（計算）
			double ExpVotes = 0, TotalVotes = 0;
			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 9; j++) {
					double keisuu = VoteRate1; if (Wakamono[i][j] == false) keisuu = VoteRate2;
					ExpVotes += keisuu / (1.0 + exp(-0.1 * Keisei[i][j]));
					TotalVotes += keisuu;
				}
			}
			int Exp1 = (int)(100.0 * ExpVotes / TotalVotes + 0.5);
			int Exp2 = 100 - Exp1;

			// 形勢評価（1列目）
			font15(U"演説力").draw(577, 414, Palette::White);
			font15(U"影響力").draw(577, 442, Palette::White);
			font15(U"協力者").draw(577, 470, Palette::White);
			font15(U"資金").draw(585, 498, Palette::White);
			font15(U"演説回数").draw(570, 526, Palette::White);
			font15(U"予測得票").draw(570, 554, Palette::White);

			// 形勢評価（2列目）
			font15(U"あなた").draw(647, 386, Palette::White);
			font15(U"Lv.").draw(697 - 9 * (3 + ToString(Z1.Speech).size()), 414, colorP1); font15(Z1.Speech).draw(693 - 9 * ToString(Z1.Speech).size(), 414, colorP1);
			font15(U"Lv.").draw(697 - 9 * (3 + ToString(Z1.Kanban).size()), 442, colorP1); font15(Z1.Kanban).draw(693 - 9 * ToString(Z1.Kanban).size(), 442, colorP1);
			font15(Z1.Jiban).draw(680 - 9 * ToString(Z1.Jiban).size(), 470, colorP1); font15(U"人").draw(680, 470, colorP1);
			font15(Z1.Kaban).draw(680 - 9 * ToString(Z1.Kaban).size(), 498, colorP1); font15(U"万").draw(680, 498, colorP1);
			font15(Z1.NumSpc).draw(680 - 9 * ToString(Z1.NumSpc).size(), 526, colorP1); font15(U"回").draw(680, 526, colorP1);
			font15(Exp1).draw(681 - 9 * ToString(Exp1).size(), 554, colorP1); font15(U"%").draw(681, 554, colorP1);

			// 形勢評価（3列目）
			font15(U"AI").draw(721, 386, Palette::White);
			font15(U"Lv.").draw(757 - 9 * (3 + ToString(Z2.Speech).size()), 414, colorP2); font15(Z2.Speech).draw(753 - 9 * ToString(Z2.Speech).size(), 414, colorP2);
			font15(U"Lv.").draw(757 - 9 * (3 + ToString(Z2.Kanban).size()), 442, colorP2); font15(Z2.Kanban).draw(753 - 9 * ToString(Z2.Kanban).size(), 442, colorP2);
			font15(Z2.Jiban).draw(740 - 9 * ToString(Z2.Jiban).size(), 470, colorP2); font15(U"人").draw(740, 470, colorP2);
			font15(Z2.Kaban).draw(740 - 9 * ToString(Z2.Kaban).size(), 498, colorP2); font15(U"万").draw(740, 498, colorP2);
			font15(Z2.NumSpc).draw(740 - 9 * ToString(Z2.NumSpc).size(), 526, colorP2); font15(U"回").draw(740, 526, colorP2);
			font15(Exp2).draw(741 - 9 * ToString(Exp2).size(), 554, colorP2); font15(U"%").draw(741, 554, colorP2);

			// 盤面の表示
			if (Situation == 3) {
				double wait_keisuu = 1.0;
				if (WaitTime <= 0.2) wait_keisuu = Min(1.00, -1.25 * WaitTime);
				for (int i = 0; i < 5; i++) {
					for (int j = 0; j < 9; j++) {
						if (Choice[i][j] == true) Rect{ 45 + j * 50, 40 + i * 50, 50 }.draw(ColorF{ 1.0, 0.6, 0.2, (0.4 + 0.6 * ButtonB[i][j]) * wait_keisuu });
					}
				}
			}
			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 9; j++) {
					double ra = 0, rb = 0, rc = 0;
					if (Keisei[i][j] >= 30) { ra = 1.00; rb = 0.10; rc = 0.55; }
					else if (Keisei[i][j] >= 15) {
						ra = 0.95 + 0.05 * (Keisei[i][j] - 15) / 15.0;
						rb = 0.35 - 0.25 * (Keisei[i][j] - 15) / 15.0;
						rc = 0.65 - 0.10 * (Keisei[i][j] - 15) / 15.0;
					}
					else if (Keisei[i][j] >= 1) {
						ra = 0.86 + 0.09 * (Keisei[i][j] - 0) / 15.0;
						rb = 0.80 - 0.45 * (Keisei[i][j] - 0) / 15.0;
						rc = 0.83 - 0.18 * (Keisei[i][j] - 0) / 15.0;
					}
					else if (Keisei[i][j] >= 0) {
						ra = 0.87;
						rb = 0.87;
						rc = 0.87;
					}
					else if (Keisei[i][j] >= -15) {
						ra = 0.81 - 0.36 * (-Keisei[i][j] - 0) / 15.0;
						rb = 0.81 - 0.36 * (-Keisei[i][j] - 0) / 15.0;
						rc = 0.83 - 0.18 * (-Keisei[i][j] - 0) / 15.0;
					}
					else if (Keisei[i][j] >= -30) {
						ra = 0.45 - 0.20 * (-Keisei[i][j] - 15) / 15.0;
						rb = 0.45 - 0.20 * (-Keisei[i][j] - 15) / 15.0;
						rc = 0.65 - 0.10 * (-Keisei[i][j] - 15) / 15.0;
					}
					else { ra = 0.25; rb = 0.25; rc = 0.55; }
					Rect{ 50 + j * 50, 45 + i * 50, 40 }.draw(ColorF{ ra, rb, rc });
					if (Wakamono[i][j] == true) font15(U"▼").draw(62 + j * 50, 45 + i * 50);
				}
			}

			// 予想投票率
			VoteRate1 = Min(99, VoteRate1);
			VoteRate2 = Min(99, VoteRate2);
			Rect{ 550, 165, 200, 120 }.drawFrame(1, 1, black02);
			Line{ 550, 73, 670, 73 }.draw(2, ColorF{ 0.00 });
			font20(U"予想投票率").draw(550, 45, black02);
			font20(U"18～39歳：").draw(575, 80, black02); font20(VoteRate1).draw(705, 80, black02); font20(U"%").draw(730, 80, black02);
			font20(U"40歳以上：").draw(575, 105, black02); font20(VoteRate2).draw(705, 105, black02); font20(U"%").draw(730, 105, black02);

			// 凡例
			font15(U"▼：若者の多い地区").draw(570, 180, black02);
			Rect{ 570, 210, 20 }.draw(colorP2);
			Rect{ 593, 210, 20 }.draw(ColorF{ 0.41, 0.41, 0.63 });
			Rect{ 616, 210, 20 }.draw(ColorF{ 0.57, 0.57, 0.71 });
			Rect{ 639, 210, 20 }.draw(ColorF{ 0.85 });
			Rect{ 662, 210, 20 }.draw(ColorF{ 0.92, 0.50, 0.71 });
			Rect{ 685, 210, 20 }.draw(ColorF{ 0.96, 0.30, 0.63 });
			Rect{ 708, 210, 20 }.draw(ColorF{ 1.00, 0.10, 0.55 });
			font15(U"AI").draw(570, 230, colorP2);
			font15(U"+30").draw(570, 246, colorP2);
			font15(U"あなた").draw(685, 230, ColorF{ 1.00, 0.10, 0.55 });
			font15(U"+30").draw(699, 246, ColorF{ 1.00, 0.10, 0.55 });
			font15(U"評価値").draw(628, 230, ColorF{ 0.50 });
			font10(U"※評価値の付け方は GitHub 参照").draw(598, 270, ColorF{ 0.50 });

			// 左側の表示
			font20(U"選挙戦").draw(15, 340, black02);
			font20((Turn + 2) / 3).draw(100 - 12 * ToString((Turn + 2) / 3).size(), 340, black02);
			font20(U"日目 [全14日]").draw(100, 340, black02);
			if (Turn % 3 == 1) font20(U"朝").draw(250, 340, black02);
			if (Turn % 3 == 2) font20(U"昼").draw(250, 340, black02);
			if (Turn % 3 == 0) font20(U"夜").draw(250, 340, black02);
			if (Situation == 2) {
				Rect{ 352, 338, 160, 30 }.draw(ColorF{ colorP2, Min(1.00, 2.0 * abs(WaitTime)) });
				font20(U"AIのターン").draw(380, 338, AlphaF(Min(1.00, 2.0 * abs(WaitTime))));
			}
			else {
				Rect{ 352, 338, 160, 30 }.draw(ColorF{ colorP1, Min(1.00, 2.0 * abs(WaitTime)) });
				font20(U"あなたのターン").draw(362, 338, AlphaF(Min(1.00, 2.0 * abs(WaitTime))));
			}

			// 下半分の描画
			if (Situation == 1 || Situation == 2) {
				int offset[6] = { 0, 0, 0, 0, 0, 0 };
				if (WaitTime >= -0.05 && WaitTime <= 1.2) {
					for (int i = 0; i < 6; i++) offset[i] = Max(0, (int)(400 * (i + 2) - 2400.0 * WaitTime));
				}

				if (Situation == 1) {
					for (int i = 0; i < 6; ++i) {
						double button = ButtonA[i];
						Rect{ 20 - offset[i], 385 + 34 * i, 24 }.draw(ColorF{ 1.00 - 0.40 * button, 1.00 - 0.90 * button, 1.00 - 0.70 * button });
					}
				}
				else {
					for (int i = 0; i < 6; ++i) {
						double button = ButtonA[i];
						Rect{ 20 - offset[i], 385 + 34 * i, 24 }.draw(ColorF{ 1.00 - 0.75 * button, 1.00 - 0.75 * button, 1.00 - 0.45 * button });
					}
				}
				auto color_frame = black02;
				for (int i = 0; i < 6; ++i) {
					Rect{ 20 - offset[i], 385 + 34 * i, 24 }.drawFrame(2, color_frame);
				}

				// 文字の表示
				font15(U"演説をする（資金 -20万、形勢向上）").draw(55 - offset[0], 385, black02);
				font15(U"演説の練習（演説力 +1）").draw(55 - offset[1], 419, black02);
				font15(U"寄付を募る（協力者×1万だけ資金増）").draw(55 - offset[2], 453, black02);
				font15(U"設備の購入（資金 -350万、演説回数 +1）").draw(55 - offset[3], 487, black02);
				font15(U"地盤の強化（協力者が 10+影響力 増える）").draw(55 - offset[4], 521, black02);
				font15(U"ＳＮＳ投稿（若者への影響力 +1）").draw(55 - offset[5], 555, black02);
			}

			// 自分の行動
			if (Situation == 1) {
				font20(U"クリックで").draw(380, 520, ColorF{ 0.2, Periodic::Sine0_1(1.5s) });
				font20(U"行動を選択！").draw(380, 550, ColorF{ 0.2, Periodic::Sine0_1(1.5s) });
				font20(U"資金が不足").draw(380, 420, ColorF{ colorP1, ButtonA[6] });
				font20(U"しています").draw(380, 450, ColorF{ colorP1, ButtonA[6] });

				// 遷移状態の場合
				if (WaitTime <= 0.20) Rect{ 0, 380, 500, 220 }.draw(ColorF{ 0.90, 0.95, 1.00, 1.0 + 2.0 * WaitTime });
				if (WaitTime >= 0.00 && IsWaiting == 1) {
					for (int i = 0; i < 14; i++) ButtonA[i] = 0;
					IsWaiting = 0; Situation = 2; NextAI = AI_Choice();
				}
				if (WaitTime >= 0.00 && IsWaiting == 2) { Situation = 3; IsWaiting = 0; Life = Min(Z1.NumSpc, Z1.Kaban / 20); ResetChoice(); }

				// 初めての場合
				if (Hajimete == true) {
					Rect{ 100, 100, 600, 400 }.draw(AlphaF(0.9));
					Rect{ 100, 100, 600, 400 }.drawFrame(3, black02);
					font50(U"選挙戦へようこそ！").draw(120, 110, black02);
					font18(U"あなたは、とある議員選挙の立候補者です。今日から 2 週間かけて").draw(120, 190, black02);
					font18(U"選挙活動を行います。選挙で大切な三バン（地盤・カバン・看板）").draw(120, 220, black02);
					font18(U"を獲得し、SNS の力も活用して有利に選挙戦を進めましょう！").draw(120, 250, black02);
					font18(U"なお、このゲームの設定は、").draw(120, 295, black02);
					font18(U"実際の選挙とは一切関係のない").draw(354, 295, Palette::Red);
					font18(U"ことに注").draw(606, 295, black02);
					font18(U"意してください。実際の選挙の方が難しいです。").draw(120, 325, black02);
					Rect{ 250, 390, 300, 70 }.draw(ColorF{ 0.40, 0.40, 0.70, 0.50 + 0.50 * ButtonA[14] });
					font30(U"はじめる").draw(340, 400, Palette::White);
				}
			}

			// AI の行動
			if (Situation == 2) {
				// ボタン・時間の更新
				WaitTime += Scene::DeltaTime();
				if (WaitTime >= 1.4) {
					ButtonA[NextAI] = 1;
					WaitTime = -2.4; IsWaiting = 1;
					if (NextAI == 0) { AI_Enzetsu(); }
					if (NextAI == 1) { Z2.Speech += 1; }
					if (NextAI == 2) { Z2.Kaban += Z2.Jiban; }
					if (NextAI == 3) { Z2.Kaban -= 350; Z2.NumSpc += 1; }
					if (NextAI == 4) { Z2.Jiban += 10 + Z2.Kanban; }
					if (NextAI == 5) { Z2.Kanban += 1; }
				}

				// 遷移状態の場合
				if (WaitTime <= 0.20) Rect{ 0, 380, 500, 220 }.draw(ColorF{ 0.90, 0.95, 1.00, 1.0 + 2.0 * WaitTime });
				if (WaitTime >= 0.00 && IsWaiting == 1) {
					for (int i = 0; i < 14; i++) ButtonA[i] = 0;
					Turn += 1; IsWaiting = 0; Situation = 1;

					if (Turn == 43) { Refresh(); Situation = 6; }
					else if (Turn % 3 == 1 && HajimeteEnzetsu == true && AllZero() == false) { Refresh(); HajimeteEnzetsu = false; Situation = 8; }
					else if (Turn % 3 == 1) { Refresh(); Situation = 4; }

					// 履歴の更新
					if (Turn % 3 == 1) {
						History[(Turn - 1) / 3] = Exp1;
						for (int i = 0; i < 5; i++) {
							for (int j = 0; j < 9; j++) History2[(Turn - 1) / 3][i][j] = Keisei[i][j];
						}
					}
				}
			}

			// 演説場所を選ぶ
			if (Situation == 3) {
				font20(U"黄色で表示されている選択肢の中から").draw(90, 500, ColorF{ 0.2, Periodic::Sine0_1(1.5s) });
				font20(U"演説する地区をクリックしてください").draw(90, 530, ColorF{ 0.2, Periodic::Sine0_1(1.5s) });
				font30(U"残り演説回数").draw(100, 430, black02);
				font60(Life).draw(310, 400, colorP1);
				font60(U"/").draw(346, 400, black02);
				font60(Z1.NumSpc).draw(382, 400, black02);
				font10(U"※演説した場所の形勢が [演説力レベル] だけ上がります。").draw(245, 575, black02);

				// 遷移状態の場合
				if (WaitTime <= 0.2) Rect{ 0, 380, 500, 220 }.draw(ColorF{ 0.90, 0.95, 1.00, Min(1.00, 1.0 + 2.0 * WaitTime) });
				if (WaitTime >= 0.0 && IsWaiting == 1) {
					for (int i = 0; i < 14; i++) ButtonA[i] = 0;
					IsWaiting = 0; Situation = 2; NextAI = AI_Choice();
				}
			}

			if (Situation == 8) {
				font20(U"一日の終わりには").draw(45, 405, black02);
				font20(U"形勢が隣に拡散します").draw(45, 435, black02);
				Rect{ 150, 500, 220, 55 }.draw(ColorF{ 1.00, 1.00 - 0.20 * ButtonA[7], 1.00 - 0.40 * ButtonA[7], Min(1.00, abs(WaitTime) * 2.0) });
				font30(U"確認する").draw(200, 505, ColorF{ 0.2, Min(1.00, abs(WaitTime) * 2.0) });
				font10(U"※ 3 日目以降も、一日の終わりには同様のことが起こります。").draw(220, 575, black02);

				// 左側の正方形
				for (int i = 0; i < 3; i++) {
					for (int j = 0; j < 3; j++) {
						double ra = 0.85, rb = 0.85, rc = 0.85;
						if (i == 1 && j == 1) { ra = 0.60; rb = 0.10; rc = 0.30; }
						Rect{ 300 + i * 20, 405 + j * 20, 16 }.draw(ColorF{ ra, rb, rc });
					}
				}

				// 右側の正方形
				for (int i = 0; i < 3; i++) {
					for (int j = 0; j < 3; j++) {
						double ra = 0.85, rb = 0.85, rc = 0.85;
						if (i == 1 && j == 1) { ra = 0.60; rb = 0.10; rc = 0.30; }
						else if (abs(i - 1) + abs(j - 1) <= 1) { ra = 0.75; rb = 0.60; rc = 0.65; }
						Rect{ 420 + i * 20, 405 + j * 20, 16 }.draw(ColorF{ ra, rb, rc });
					}
				}

				// 矢印を描く
				Rect{ 370, 424, 30, 12 }.draw(black02);
				Triangle(400, 420, 400, 440, 415, 430).draw(black02);

				// 遷移状態の場合
				if (WaitTime >= 0.0 && IsWaiting == 1) {
					IsWaiting = 0; Situation = 4;
				}
			}

			if (Situation == 4) {
				font20(U"深夜に、ＳＮＳを通じた選挙活動が").draw(100, 420, ColorF{ 0.2, Min(1.00, abs(WaitTime) * 2.0) });
				font20(U"若者に対して行われました").draw(140, 450, ColorF{ 0.2, Min(1.00, abs(WaitTime) * 2.0) });
				Rect{ 150, 500, 220, 55 }.draw(ColorF{ 1.00, 1.00 - 0.20 * ButtonA[7], 1.00 - 0.40 * ButtonA[7], Min(1.00, abs(WaitTime) * 2.0) });
				font30(U"確認する").draw(200, 505, ColorF{ 0.2, Min(1.00, abs(WaitTime) * 2.0) });
				font10(U"※若者の多い地区 1 箇所の形勢が [影響力レベル] だけ上がります。").draw(200, 575, black02);

				// 遷移状態の場合
				if (WaitTime >= 0.0 && IsWaiting == 1) {
					IsWaiting = 0; Situation = 5; Card = 0; CardID = -1;
					Apply_SNS();
				}
			}

			if (Situation == 5) {
				font20(U"選挙戦").draw(110, 420, ColorF{ 0.2, Min(1.00, abs(WaitTime) * 2.0) });
				font20((Turn + 2) / 3).draw(193 - 6 * ToString((Turn + 2) / 3).size(), 420, ColorF{ 0.2, Min(1.00, abs(WaitTime) * 2.0) });
				font20(U"日目が始まりました！").draw(210, 420, ColorF{ 0.2, Min(1.00, abs(WaitTime) * 2.0) });
				font20(U"「チャンスカード」を引いてください").draw(90, 450, ColorF{ 0.2, Min(1.00, abs(WaitTime) * 2.0) });
				Rect{ 150, 500, 220, 55 }.draw(ColorF{ 1.00, 1.00 - 0.20 * ButtonA[7], 1.00 - 0.40 * ButtonA[7], Min(1.00, abs(WaitTime) * 2.0) });
				font30(U"カードを引く").draw(170, 505, ColorF{ 0.2, Min(1.00, abs(WaitTime) * 2.0) });

				// カードがある場合
				if (Card == 1 && WaitTime >= 0.05) {
					Rect{ 240, 180, 320, 240 }.draw(ColorF{ 1.0, 1.0, 0.5, 0.9 });
					Rect{ 240, 180, 320, 240 }.drawFrame(3, black02);

					if (CardID == 0) {
						font25(U"選挙への関心").draw(250, 190, black02);
						font15(U"若者の投票率が 10% 上昇する。").draw(250, 240, black02);
					}
					if (CardID == 1) {
						font25(U"物価の上昇").draw(250, 190, black02);
						font15(U"両候補の資金が 40 万円減少する。").draw(250, 240, black02);
					}
					if (CardID == 2) {
						font25(U"テレビ出演").draw(250, 190, black02);
						font15(U"影響力レベルが 2 上昇する。").draw(250, 240, black02);
						font15(U"投票率が 4% 上昇する。").draw(250, 270, black02);
					}
					if (CardID == 3) {
						font25(U"対立候補が新聞掲載").draw(250, 190, black02);
						font15(U"相手の影響力レベルが 2 上昇する。").draw(250, 240, black02);
						font15(U"投票率が 3% 上昇する。").draw(250, 270, black02);
					}
					if (CardID == 4) {
						font25(U"人柄が評価される").draw(250, 190, black02);
						font15(U"すべての地区における自分の形勢が").draw(250, 240, black02);
						font15(U"1 ポイント上昇する。").draw(250, 270, black02);
					}
					if (CardID == 5) {
						font25(U"まさかの失言！？").draw(250, 190, black02);
						font15(U"すべての地区における自分の形勢が").draw(250, 240, black02);
						font15(U"1 ポイント低下する。").draw(250, 270, black02);
					}
					if (CardID == 6) {
						font25(U"推薦状が得られた！").draw(250, 190, black02);
						font15(U"協力者が 25 人増える。").draw(250, 240, black02);
					}
					if (CardID == 7) {
						font25(U"相手が後援会を作った！").draw(250, 190, black02);
						font15(U"相手の協力者が 25 人増える。").draw(250, 240, black02);
					}
					if (CardID == 8) {
						font25(U"宝くじで当選").draw(250, 190, black02);
						font15(U"資金が 70 万円増える。").draw(250, 240, black02);
					}
					if (CardID == 9) {
						font25(U"交通事故発生").draw(250, 190, black02);
						font15(U"選挙カーが事故に遭う。").draw(250, 240, black02);
						font15(U"70 万円支払う。").draw(250, 270, black02);
					}
					Rect{ 290, 330, 220, 55 }.draw(ColorF{ 1.00, 1.00 - 0.20 * ButtonA[8], 1.00 - 0.40 * ButtonA[8], Min(1.00, abs(WaitTime) * 2.0) });
					font30(U"確認する").draw(340, 335, ColorF{ 0.2, Min(1.00, abs(WaitTime) * 2.0) });
				}

				// 遷移状態の場合
				if (WaitTime <= 0.2) Rect{ 0, 380, 500, 220 }.draw(ColorF{ 0.90, 0.95, 1.00, Min(1.00, 1.0 + 2.0 * WaitTime) });
				if (WaitTime >= 0.0 && IsWaiting == 1) {
					IsWaiting = 0; Situation = 1; Card = 0;
					if (CardID == 0) { VoteRate1 += 10; }
					if (CardID == 1) { Z1.Kaban = Max(0, Z1.Kaban - 40); Z2.Kaban = Max(0, Z2.Kaban - 40); }
					if (CardID == 2) { Z1.Kanban += 2; VoteRate1 += 4; VoteRate2 += 4; }
					if (CardID == 3) { Z2.Kanban += 2; VoteRate1 += 3; VoteRate2 += 3; }
					if (CardID == 4) { for (int i = 0; i < 45; i++) Keisei[i / 9][i % 9] += 1; }
					if (CardID == 5) { for (int i = 0; i < 45; i++) Keisei[i / 9][i % 9] -= 1; }
					if (CardID == 6) { Z1.Jiban += 25; }
					if (CardID == 7) { Z2.Jiban += 25; }
					if (CardID == 8) { Z1.Kaban += 70; }
					if (CardID == 9) { Z1.Kaban = Max(0, Z1.Kaban - 70); }
				}
			}

			if (Situation == 6) {
				font20(U"いよいよ投票日です！").draw(160, 420, ColorF{ 0.2, Min(1.00, abs(WaitTime) * 2.0) });
				Rect{ 150, 500, 220, 55 }.draw(ColorF{ 1.00, 1.00 - 0.20 * ButtonA[7], 1.00 - 0.40 * ButtonA[7], Min(1.00, abs(WaitTime) * 2.0) });
				font30(U"開票に進む").draw(185, 505, ColorF{ 0.2, Min(1.00, abs(WaitTime) * 2.0) });

				// 遷移状態の場合
				if (WaitTime <= 0.2 && IsWaiting == 1) Rect{ 0, 0, 800, 600 }.draw(AlphaF(Min(1.00, 1.0 + 1.25 * WaitTime)));
				if (WaitTime >= 0.0 && IsWaiting == 1) {
					IsWaiting = 0; Situation = 7; WaitTime = 0; Vote_Kakutei();
				}
			}


			// ################################################
			// ##                                            ##
			// ##            以下、マウスの情報              ##
			// ##                                            ##
			// ################################################
			if (Situation == 1 && Hajimete == true) {
				// マウスの状態
				int MouseState = -1;

				if (Rect{ 250, 390, 300, 170 }.mouseOver()) MouseState = 0;
				if (MouseState == 0)
					ButtonA[14] = Min(1.0, ButtonA[14] + 5.0 * Scene::DeltaTime());
				else
					ButtonA[14] = Max(0.0, ButtonA[14] - 5.0 * Scene::DeltaTime());

				// クリックの状態
				if (Scene::Time() - GetLastClick >= 0.1 && MouseL.down()) {
					GetLastClick = Scene::Time();
					if (MouseState == 0) Hajimete = false;
				}
			}
			if (Situation == 1 && Hajimete == false) {
				// マウスの状態
				int MouseState = -1;
				for (int i = 0; i < 6; ++i) {
					if (Rect{ 20, 385 + 34 * i, 24 }.mouseOver()) {
						MouseState = i;
						break;
					}
				}

				// ボタン・時間の更新
				for (int i = 0; i < 6; i++) {
					ButtonA[i] = i != MouseState
						? Max(0.0, ButtonA[i] - 5.0 * Scene::DeltaTime())
						: Min(1.0, ButtonA[i] + 5.0 * Scene::DeltaTime());
				}
				ButtonA[6] = Max(0.0, ButtonA[6] - 1.0 * Scene::DeltaTime());
				WaitTime += Scene::DeltaTime();

				// クリックの状態
				if (Scene::Time() - GetLastClick >= 0.1 && MouseL.down() && WaitTime >= 1.2) {
					GetLastClick = Scene::Time();
					bool NextMove = false;
					if (MouseState == 0) {
						if (Z1.Kaban >= 20) { NextMove = true; }
						else ButtonA[6] = 1.0;
					}
					if (MouseState == 3) {
						if (Z1.Kaban >= 350) { NextMove = true; Z1.Kaban -= 350; Z1.NumSpc += 1; }
						else ButtonA[6] = 1.0;
					}
					if (MouseState == 1) { Z1.Speech += 1; NextMove = true; }
					if (MouseState == 2) { Z1.Kaban += Z1.Jiban; NextMove = true; }
					if (MouseState == 4) { Z1.Jiban += 10 + Z1.Kanban; NextMove = true; }
					if (MouseState == 5) { Z1.Kanban += 1; NextMove = true; }
					if (NextMove == true) {
						WaitTime = -0.8;
						if (MouseState == 0) IsWaiting = 2;
						if (MouseState != 0) IsWaiting = 1;
					}
				}
			}

			if (Situation == 3) {
				int StateX = -1, StateY = -1;
				if (Rect{ 45, 40, 450 }.mouseOver()) {
					StateY = Clamp((int)((Cursor::PosF().x - 45.0) / 50.0), 0, 8);
					StateX = Clamp((int)((Cursor::PosF().y - 40.0) / 50.0), 0, 4);
				}
				for (int i = 0; i < 5; i++) {
					for (int j = 0; j < 9; j++) {
						ButtonB[i][j] = (i == StateX && j == StateY)
							? Min(1.0, ButtonB[i][j] + 5.0 * Scene::DeltaTime())
							: Max(0.0, ButtonB[i][j] - 5.0 * Scene::DeltaTime());
					}
				}
				WaitTime += Scene::DeltaTime();

				// クリックの状態
				if (Scene::Time() - GetLastClick >= 0.1 && MouseL.down() && WaitTime >= 0.0) {
					GetLastClick = Scene::Time();
					if (StateX != -1 && Choice[StateX][StateY] == true) {
						Keisei[StateX][StateY] += Z1.Speech;
						Life -= 1;
						Z1.Kaban -= 20;
						if (Life == 0) { WaitTime = -0.8; IsWaiting = 1; }
					}
				}
			}

			if (Situation == 8) {
				int MouseState = -1;
				if (Rect{ 150, 500, 220, 55 }.mouseOver()) MouseState = 0;
				ButtonA[7] = MouseState == 0
					? Min(1.0, ButtonA[7] + 5.0 * Scene::DeltaTime())
					: Max(0.0, ButtonA[7] - 5.0 * Scene::DeltaTime());
				WaitTime += Scene::DeltaTime();

				// クリックの状態
				if (Scene::Time() - GetLastClick >= 0.1 && MouseL.down() && WaitTime >= 0.0) {
					GetLastClick = Scene::Time();
					if (MouseState == 0) { WaitTime = -0.45; IsWaiting = 1; }
				}
			}

			if (Situation == 4) {
				int MouseState = -1;
				if (Rect{ 150, 500, 220, 55 }.mouseOver()) MouseState = 0;
				ButtonA[7] = MouseState == 0
					? Min(1.0, ButtonA[7] + 5.0 * Scene::DeltaTime())
					: Max(0.0, ButtonA[7] - 5.0 * Scene::DeltaTime());
				WaitTime += Scene::DeltaTime();

				// クリックの状態
				if (Scene::Time() - GetLastClick >= 0.1 && MouseL.down() && WaitTime >= 0.0) {
					GetLastClick = Scene::Time();
					if (MouseState == 0) { WaitTime = -0.45; IsWaiting = 1; }
				}
			}

			if (Situation == 5) {
				int MouseState = -1;
				if (Rect{ 150, 500, 220, 55 }.mouseOver()) MouseState = 0;
				if (Rect{ 290, 330, 220, 55 }.mouseOver()) MouseState = 1;
				if (MouseState == 0 && Card == 0) ButtonA[7] = Min(1.0, ButtonA[7] + 5.0 * Scene::DeltaTime());
				if (MouseState != 0 || Card != 0) ButtonA[7] = Max(0.0, ButtonA[7] - 5.0 * Scene::DeltaTime());
				if (MouseState == 1 && Card == 1) ButtonA[8] = Min(1.0, ButtonA[8] + 5.0 * Scene::DeltaTime());
				if (MouseState != 1 || Card != 1) ButtonA[8] = Max(0.0, ButtonA[8] - 5.0 * Scene::DeltaTime());
				WaitTime += Scene::DeltaTime();

				// クリックの状態
				if (Scene::Time() - GetLastClick >= 0.1 && MouseL.down() && WaitTime >= 0.0) {
					GetLastClick = Scene::Time();
					if (MouseState == 0 && Card == 0) {
						Card = 1;
						int val = Random(199);
						if (val < 92) CardID = val / 23;
						else if (val < 100) CardID = 4;
						else if (val < 108) CardID = 5;
						else CardID = (val - 108) / 23 + 6;
					}
					if (MouseState == 1 && Card == 1) { WaitTime = -0.45; IsWaiting = 1; }
				}
			}

			if (Situation == 6) {
				int MouseState = -1;
				if (Rect{ 150, 500, 220, 55 }.mouseOver()) MouseState = 0;
				if (MouseState == 0) ButtonA[7] = Min(1.0, ButtonA[7] + 5.0 * Scene::DeltaTime());
				if (MouseState != 0) ButtonA[7] = Max(0.0, ButtonA[7] - 5.0 * Scene::DeltaTime());
				WaitTime += Scene::DeltaTime();

				// クリックの状態
				if (Scene::Time() - GetLastClick >= 0.1 && MouseL.down() && WaitTime >= 0.0) {
					GetLastClick = Scene::Time();
					if (MouseState == 0) { WaitTime = -0.80; IsWaiting = 1; }
				}
			}
		}


		// -----------------------------------------------------------------------------------------------------------------------
		// ------------------------------------------------- 開票中 --------------------------------------------------------------
		// -----------------------------------------------------------------------------------------------------------------------
		if (Situation == 7) {
			font60(U"開票速報").draw(20, 10, black02);
			font20(U"投票率").draw(520, 12, black02);
			font20(U"18～39歳").draw(600, 12, black02);
			font20(U"40歳以上").draw(600, 36, black02);
			font20(U"全体　　").draw(600, 60, black02);
			font20(U"：", FinalVote1 / 100).draw(690, 12, black02); font20(U".").draw(734, 12, black02); if (FinalVote1 % 100 < 10) { font20(U"0", FinalVote1 % 100, U"%").draw(742, 12, black02); }
			else { font20(FinalVote1 % 100, U"%").draw(742, 12, black02); }
			font20(U"：", FinalVote2 / 100).draw(690, 36, black02); font20(U".").draw(734, 36, black02); if (FinalVote2 % 100 < 10) { font20(U"0", FinalVote2 % 100, U"%").draw(742, 36, black02); }
			else { font20(FinalVote2 % 100, U"%").draw(742, 36, black02); }
			font20(U"：", FinalVote3 / 100).draw(690, 60, black02); font20(U".").draw(734, 60, black02); if (FinalVote3 % 100 < 10) { font20(U"0", FinalVote3 % 100, U"%").draw(742, 60, black02); }
			else { font20(FinalVote3 % 100, U"%").draw(742, 60, black02); }

			// 時刻の更新
			WaitTime += Scene::DeltaTime();
			while (WaitTime >= 1.0 && Kakutei < 45) { Kakutei += 1; WaitTime -= 1.0; }

			// 開票速報（予測）
			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 9; j++) {
					double ra = 0, rb = 0, rc = 0;
					if (Keisei[i][j] >= 30) { ra = 1.00; rb = 0.10; rc = 0.55; }
					else if (Keisei[i][j] >= 15) {
						ra = 0.95 + 0.05 * (Keisei[i][j] - 15) / 15.0;
						rb = 0.35 - 0.25 * (Keisei[i][j] - 15) / 15.0;
						rc = 0.65 - 0.10 * (Keisei[i][j] - 15) / 15.0;
					}
					else if (Keisei[i][j] >= 1) {
						ra = 0.86 + 0.09 * (Keisei[i][j] - 0) / 15.0;
						rb = 0.80 - 0.45 * (Keisei[i][j] - 0) / 15.0;
						rc = 0.83 - 0.18 * (Keisei[i][j] - 0) / 15.0;
					}
					else if (Keisei[i][j] >= 0) {
						ra = 0.87;
						rb = 0.87;
						rc = 0.87;
					}
					else if (Keisei[i][j] >= -15) {
						ra = 0.81 - 0.36 * (-Keisei[i][j] - 0) / 15.0;
						rb = 0.81 - 0.36 * (-Keisei[i][j] - 0) / 15.0;
						rc = 0.83 - 0.18 * (-Keisei[i][j] - 0) / 15.0;
					}
					else if (Keisei[i][j] >= -30) {
						ra = 0.45 - 0.20 * (-Keisei[i][j] - 15) / 15.0;
						rb = 0.45 - 0.20 * (-Keisei[i][j] - 15) / 15.0;
						rc = 0.65 - 0.10 * (-Keisei[i][j] - 15) / 15.0;
					}
					else { ra = 0.25; rb = 0.25; rc = 0.55; }
					Rect{ 60 + j * 32, 120 + i * 32, 24 }.draw(ColorF{ ra, rb, rc });
					if (Wakamono[i][j] == true) font6(U"▼").draw(69 + j * 32, 120 + i * 32, Palette::White);
				}
			}
			Rect{ 120, 288, 160, 75 }.draw(black02);
			font30(U"予測").draw(170, 290, Palette::White);
			font15(U"有権者は１万人／区画").draw(125, 330, Palette::White);

			// 開票結果（実際）
			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 9; j++) {
					double ra = 0, rb = 0, rc = 0;
					if (i * 9 + j >= Kakutei || P1[i][j] == P2[i][j]) { ra = 0.85; rb = 0.85; rc = 0.85; }
					else if (P1[i][j] > P2[i][j]) { ra = 1.00; rb = 0.10; rc = 0.55; }
					else { ra = 0.25; rb = 0.25; rc = 0.55; }
					Rect{ 460 + j * 32, 120 + i * 32, 24 }.draw(ColorF{ ra, rb, rc });
					if (i * 9 + j < Kakutei) {
						int wari = (int)(100.0 * P1[i][j] / (P1[i][j] + P2[i][j]) + 0.5); wari = Min(99, wari);
						if (wari < 10) font15(wari).draw(472 + j * 32, 120 + i * 32, Palette::White);
						else font15(wari).draw(463 + j * 32, 120 + i * 32, Palette::White);
					}
				}
			}
			Rect{ 520, 288, 160, 75 }.draw(black02);
			font30(U"確定").draw(570, 290, Palette::White);
			font15(U"数字はあなたの得票率").draw(525, 330, Palette::White);

			// 矢印を描く
			Rect{ 360, 193, 60, 14 }.draw(black02);
			Triangle(420, 185, 420, 215, 440, 200).draw(black02);

			// 現在の票数
			int CurrentVote1 = 0, CurrentVote2 = 0;
			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 9; j++) {
					if (i * 9 + j < Kakutei) {
						CurrentVote1 += P1[i][j];
						CurrentVote2 += P2[i][j];
					}
				}
			}
			DisVote1 += (CurrentVote1 - DisVote1 + 5) / 6;
			DisVote2 += (CurrentVote2 - DisVote2 + 5) / 6;

			// 票数のグラフ
			RectF{ 40, 510, 720.0 * DisVote1 / TotalVote, 60 }.draw(colorP1);
			RectF{ 760.0 - 720.0 * DisVote2 / TotalVote, 510, 720.0 * DisVote2 / TotalVote, 60 }.draw(colorP2);
			Rect{ 40, 510, 720, 60 }.drawFrame(3, black02);
			Line{ 400, 510, 400, 570 }.draw(LineStyle::SquareDot, 2, Color(20, 20, 20)); font20(U"▼").draw(390, 485, black02);

			// 赤の票数
			font50(DisVote1).draw(40, 440, colorP1);
			font30(U"票").draw(40 + 32 * ToString(DisVote1).size(), 460, colorP1);

			// 青の票数
			font50(DisVote2).draw(730 - 32 * ToString(DisVote2).size(), 440, colorP2);
			font30(U"票").draw(730, 460, colorP2);

			// 勝敗確定
			if (WaitTime >= 2.5) {
				int wari1 = (int)(100.0 * CurrentVote1 / (CurrentVote1 + CurrentVote2) + 0.5);
				int wari2 = 100 - wari1;

				if (CurrentVote1 >= CurrentVote2) {
					Rect{ 0, 0, 800, 600 }.draw(ColorF{ colorP1, 0.90 });
					font80(U"当選！").draw(280, 30, Palette::White);
					Rect{ 200, 400, 400, 100 }.draw(AlphaF(0.5 + 0.5 * ButtonA[9]));
					font50(U"結果画面へ").draw(275, 416, colorP1);
				}
				else {
					Rect{ 0, 0, 800, 600 }.draw(ColorF{ colorP2, 0.90 });
					font80(U"落選…").draw(280, 30, Palette::White);
					Rect{ 200, 400, 400, 100 }.draw(AlphaF(0.5 + 0.5 * ButtonA[9]));
					font50(U"結果画面へ").draw(275, 416, colorP2);
				}
				font30(U"あなた").draw(160, 200, Palette::White);
				font30(U"AI").draw(160, 240, Palette::White);
				font30(U"：").draw(240, 200, Palette::White);
				font30(U"：").draw(240, 240, Palette::White);
				font30(CurrentVote1, U"票（", wari1, U"%）").draw(480 - 19 * ToString(CurrentVote1).size(), 200);
				font30(CurrentVote2, U"票（", wari2, U"%）").draw(480 - 19 * ToString(CurrentVote1).size(), 240);
				History[14] = wari1;
			}

			// マウス判定
			int MouseState = -1;
			if (Rect{ 200, 400, 400, 100 }.mouseOver()) MouseState = 0;
			if (MouseState == 0) ButtonA[9] = Min(1.0, ButtonA[9] + 5.0 * Scene::DeltaTime());
			if (MouseState != 0) ButtonA[9] = Max(0.0, ButtonA[9] - 5.0 * Scene::DeltaTime());

			// クリックの状態
			if (Scene::Time() - GetLastClick >= 0.1 && MouseL.down() && WaitTime >= 0.0) {
				GetLastClick = Scene::Time();
				if (MouseState == 0) { WaitTime = 0; Situation = 9; }
			}
		}

		// -----------------------------------------------------------------------------------------------------------------------
		// ------------------------------------------------- 開票終了 ------------------------------------------------------------
		// -----------------------------------------------------------------------------------------------------------------------
		if (Situation == 9) {
			font60(U"結果発表").draw(20, 10, black02);
			WaitTime += Scene::DeltaTime();

			// 票数のグラフ
			RectF{ 40, 510, 720.0 * DisVote1 / TotalVote, 60 }.draw(colorP1);
			RectF{ 760.0 - 720.0 * DisVote2 / TotalVote, 510, 720.0 * DisVote2 / TotalVote, 60 }.draw(colorP2);
			Rect{ 40, 510, 720, 60 }.drawFrame(3, black02);
			Line{ 400, 510, 400, 570 }.draw(LineStyle::SquareDot, 2, Color(20, 20, 20)); font20(U"▼").draw(390, 485, black02);
			font50(DisVote1).draw(40, 440, colorP1);
			font30(U"票").draw(40 + 32 * ToString(DisVote1).size(), 460, colorP1);
			font50(DisVote2).draw(730 - 32 * ToString(DisVote2).size(), 440, colorP2);
			font30(U"票").draw(730, 460, colorP2);

			// 形勢の変動
			for (int i = 0; i < 14; i++) {
				if (WaitTime < 0.1 * i) continue;
				double ax = 80.0 + 300.0 * (i + 0) / 14, ay = 360.0 - 220.0 * History[i + 0] / 100; ax = Min(ax, 80.0 + 3000.0 * WaitTime / 14.0);
				double bx = 80.0 + 300.0 * (i + 1) / 14, by = 360.0 - 220.0 * History[i + 1] / 100; bx = Min(bx, 80.0 + 3000.0 * WaitTime / 14.0);
				Quad{ Vec2{ ax, ay }, Vec2{ bx, by }, Vec2{ bx, 360.0 }, Vec2{ ax, 360.0 } }.draw(ColorF{ colorP1, 0.70 });
				Quad{ Vec2{ ax, ay }, Vec2{ bx, by }, Vec2{ bx, 140.0 }, Vec2{ ax, 140.0 } }.draw(ColorF{ colorP2, 0.70 });
			}
			Rect{ 80, 140, 300, 220 }.drawFrame(3, black02);
			Line{ 80, 195, 380, 195 }.draw(LineStyle::SquareDot, 2, black02); font15(U"75%").draw(45, 185, black02);
			Line{ 80, 250, 380, 250 }.draw(LineStyle::SquareDot, 2, black02); font15(U"50%").draw(45, 240, black02);
			Line{ 80, 305, 380, 305 }.draw(LineStyle::SquareDot, 2, black02); font15(U"25%").draw(45, 295, black02);
			font15(U"0日目").draw(80, 365, black02);
			font15(U"7日目").draw(210, 365, black02);
			font15(U"14日目").draw(335, 365, black02);

			// 勝率の変動
			int Amari = Min(14, (int)(3.0 * WaitTime) % 20);
			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 9; j++) {
					double ra = 0, rb = 0, rc = 0;
					if (History2[Amari][i][j] >= 30) { ra = 1.00; rb = 0.10; rc = 0.55; }
					else if (History2[Amari][i][j] >= 15) {
						ra = 0.95 + 0.05 * (History2[Amari][i][j] - 15) / 15.0;
						rb = 0.35 - 0.25 * (History2[Amari][i][j] - 15) / 15.0;
						rc = 0.65 - 0.10 * (History2[Amari][i][j] - 15) / 15.0;
					}
					else if (History2[Amari][i][j] >= 1) {
						ra = 0.86 + 0.09 * (History2[Amari][i][j] - 0) / 15.0;
						rb = 0.80 - 0.45 * (History2[Amari][i][j] - 0) / 15.0;
						rc = 0.83 - 0.18 * (History2[Amari][i][j] - 0) / 15.0;
					}
					else if (History2[Amari][i][j] >= 0) {
						ra = 0.87;
						rb = 0.87;
						rc = 0.87;
					}
					else if (History2[Amari][i][j] >= -15) {
						ra = 0.81 - 0.36 * (-History2[Amari][i][j] - 0) / 15.0;
						rb = 0.81 - 0.36 * (-History2[Amari][i][j] - 0) / 15.0;
						rc = 0.83 - 0.18 * (-History2[Amari][i][j] - 0) / 15.0;
					}
					else if (History2[Amari][i][j] >= -30) {
						ra = 0.45 - 0.20 * (-History2[Amari][i][j] - 15) / 15.0;
						rb = 0.45 - 0.20 * (-History2[Amari][i][j] - 15) / 15.0;
						rc = 0.65 - 0.10 * (-History2[Amari][i][j] - 15) / 15.0;
					}
					else { ra = 0.25; rb = 0.25; rc = 0.55; }
					Rect{ 440 + j * 32, 210 + i * 32, 24 }.draw(ColorF{ ra, rb, rc });
				}
			}
			font30(Amari).draw(440, 160, black02);
			font20(U"日目時点での形勢").draw(440 + 20 * ToString(Amari).size(), 170, black02);

			// 終了ボタン
			Rect{ 650, 20, 130, 60 }.draw(ColorF{ 1.0, 0.5, 0.5, 0.5 + 0.5 * ButtonA[15] });
			font30(U"終了").draw(685, 30, Palette::Black);

			// マウス判定
			int MouseState = -1;
			if (Rect{ 650, 20, 130, 60 }.mouseOver()) MouseState = 0;
			ButtonA[15] = MouseState == 0
				? Min(1.0, ButtonA[7] + 5.0 * Scene::DeltaTime())
				: Max(0.0, ButtonA[7] - 5.0 * Scene::DeltaTime());

			// クリックの状態
			if (Scene::Time() - GetLastClick >= 0.1 && MouseL.down() && WaitTime >= 0.0) {
				GetLastClick = Scene::Time();
				if (MouseState == 0) { break; }
			}
		}
	}
}
