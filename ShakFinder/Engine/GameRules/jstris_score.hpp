#pragma once

#include "tetris_points.hpp"


class JstrisScore : public Points {
public:
	
	virtual int calculate(PieceStats stats) override {
		return jstris_score(stats, state);
	}
	
private:

	struct JstrisStats {
		int combo = 0;
		bool b2b = false;
	}state;

	int jstris_score(PieceStats params, JstrisStats& state) {
		int score_acc = 50 * state.combo + 3000 * params.pc;

		switch (params.linesCleared)
		{
		case 0: {
			score_acc = 0;
			state.combo = 0;
		} break;

		case 1: {
			state.combo++;
			if (params.spin == Spin::mini) {
				// t-spin mini
				score_acc += state.b2b ? 300 : 200;
			}
			else if (params.spin == Spin::normal) {
				// t-spin single
				score_acc += state.b2b ? 1200 : 800;
			}
			else {
				// single
				score_acc += 100;
			}
		} break;
		case 2: {
			state.combo++;
			if (params.spin == Spin::mini || params.spin == Spin::normal) {
				// t-spin mini double and t-spin double are the same
				score_acc += state.b2b ? 1800 : 1200;
			}
			else {
				// double
				score_acc += 300;
			}
		} break;
		case 3: {
			state.combo++;
			if (params.spin == Spin::mini || params.spin == Spin::normal) {
				// t-spin mini double and t-spin double are the same
				score_acc += state.b2b ? 2400 : 1600;
			}
			else {
				// double
				score_acc += 500;
			}
		} break;

		case 4: {
			state.combo++;
			score_acc += state.b2b ? 3000 : 2000;
		} break;
		default:
			break;
		}


		if (params.spin == Spin::normal || params.spin == Spin::mini || params.linesCleared == 4) {
			state.b2b = true;
		}
		else {
			state.b2b = false;
		}

		return score_acc;
	}
};
