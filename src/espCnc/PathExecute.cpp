#include "PathExecute.h"

void PathExecutor::execute(GCodeCommand cmd) {

    if(cmd.code == GCode::G0RapidTravel || cmd.code == GCode::G1LinearInterpolation) {
                int here_x = motors[0].microsteps_for_pos(X);
        int here_y = motors[1].microsteps_for_pos(Y);
        int here_z = motors[2].microsteps_for_pos(Z);

        float end_X = X;
        float end_Y = Y;
        float end_Z = Z;
        if(mode == CoordinateMode::Absolute) {
            if(cmd.coord.mask & (1 << 0)) end_X = cmd.coord.X;
            if(cmd.coord.mask & (1 << 1)) end_Y = cmd.coord.Y;
            if(cmd.coord.mask & (1 << 2)) end_Z = cmd.coord.Z;
        } else {
            if(cmd.coord.mask & (1 << 0)) end_X = X + cmd.coord.X;
            if(cmd.coord.mask & (1 << 1)) end_Y = Z + cmd.coord.Y;
            if(cmd.coord.mask & (1 << 2)) end_Z = Y + cmd.coord.Z;
        }

        int target_x = motors[0].microsteps_for_pos(end_X);
        int target_y = motors[1].microsteps_for_pos(end_Y);
        int target_z = motors[2].microsteps_for_pos(end_Z);

        if(cmd.code == GCode::G0RapidTravel) {
            fast_linear_step(target_x - here_x, target_y - here_y, target_z - here_z);
        } else {
            int x_rate = motors[0].microsteps_per_sec();
            int y_rate = motors[1].microsteps_per_sec();
            int z_rate = motors[2].microsteps_per_sec();
            linear_step(target_x - here_x, target_y - here_y, target_z - here_z, x_rate, y_rate, z_rate);
        }
        


        X = end_X;
        Y = end_Y;
        Z = end_Z;
    }
    if(cmd.code == GCode::G3CircularInterpolationCounterClockwise || cmd.code == GCode::G2CircularInterpolationClockwise) {
        circular_arc(cmd.code == GCode::G3CircularInterpolationCounterClockwise, cmd.coord);
    }
    if(cmd.code == GCode::G90AbsoluteMode) {
        mode = CoordinateMode::Absolute;
    }
    if(cmd.code == GCode::G91IncrementalMode) {
        mode = CoordinateMode::Absolute;
    }
    if(cmd.code == GCode::G17XYPlaneSelection) {
        plane_mode = PlaneReferenceMode::XY;
    }
    if(cmd.code == GCode::G18XZPlaneSelection) {
        plane_mode = PlaneReferenceMode::XZ;
    }
    if(cmd.code == GCode::G19YZPlaneSelection) {
        plane_mode = PlaneReferenceMode::YZ;
    }
}

// void PathExecutor::arc_step(int dx, int dy, int di, int dj) {
//     int x_rate = motors[0].microsteps_per_sec();
//     int y_rate = motors[1].microsteps_per_sec();
//     int z_rate = motors[2].microsteps_per_sec();


// }

void PathExecutor::circular_arc(bool counterclockwise, GCodeCoordinate coord) {
    int ax0 = (plane_mode == PlaneReferenceMode::YZ) ? 1 : 0;
    int ax1 = (plane_mode == PlaneReferenceMode::XY) ? 1 : 2;
    int ax2 = 3 - ax0 - ax1;

    if(!(coord.has_coord(ax0) || coord.has_coord(ax1)) || !(coord.has_offset(ax0) || coord.has_offset(ax1))) {
        coord.print();
        Serial.println((int)plane_mode);
        panic(9);
    }

    int rates[] = {
        motors[ax0].microsteps_per_sec(),
        motors[ax1].microsteps_per_sec(),
        motors[ax2].microsteps_per_sec(),
    };
    float offset_ax0 = coord.get_offset(ax0);
    float offset_ax1 = coord.get_offset(ax1);
    float center_ax0 = get_pos(ax0) + offset_ax0;
    float center_ax1 = get_pos(ax1) + offset_ax1;
    float radius = sqrtf(offset_ax0 * offset_ax0 + offset_ax1 * offset_ax1);
    float theta0 = atan2f(-offset_ax1, -offset_ax0);
    float end_ax0 = get_pos(ax0);
    float end_ax1 = get_pos(ax1);
    float end_ax2 = get_pos(ax2);
    if(mode == CoordinateMode::Absolute) {
        end_ax0 = coord.has_coord(ax0) ? coord.get_coord(ax0) : get_pos(ax0);
        end_ax1 = coord.has_coord(ax1) ? coord.get_coord(ax1) : get_pos(ax1);
        if(coord.has_coord(ax2)) {
            end_ax2 = coord.get_coord(ax2);
        }
    } else {
        end_ax0 += coord.get_coord(ax0);
        end_ax1 += coord.get_coord(ax1);
        if(coord.has_coord(ax2)) {
            end_ax2 += coord.get_coord(ax2);
        }
    }

    float theta1 = atan2f(end_ax1 - center_ax1, end_ax0 - center_ax0);
    if(counterclockwise){
        if(theta1 < theta0) theta1 += 2 * PI; //enforce counterclockwise
    } else {
        if(theta1 > theta0) theta1 -= 2 * PI; //enforce clockwise
    }
    constexpr int steps = 128;
    float theta_step = (theta1 - theta0) / steps;
    float ax2_step = (end_ax2 - get_pos(ax2)) / steps;
    for(int i = 0; i < steps; i++) {
        float t_next = theta0 + (i+1) * theta_step;
        float next_ax0 = center_ax0 + radius * cosf(t_next);
        float next_ax1 = center_ax1 + radius * sinf(t_next);
        float next_ax2 = get_pos(ax2) + ax2_step;
        int target_ax0 = motors[ax0].microsteps_for_pos(next_ax0);
        int target_ax1 = motors[ax1].microsteps_for_pos(next_ax1);
        int target_ax2 = motors[ax2].microsteps_for_pos(next_ax2);
        int here_ax0 = motors[ax0].microsteps_for_pos(get_pos(ax0));
        int here_ax1 = motors[ax1].microsteps_for_pos(get_pos(ax1));
        int here_ax2 = motors[ax2].microsteps_for_pos(get_pos(ax2));
        int dpos[] = {
            target_ax0 - here_ax0,
            target_ax1 - here_ax1,
            target_ax2 - here_ax2
        };

        swizzle_linear_step(dpos, rates, ax0, ax1, ax2);

        set_pos(ax0, next_ax0);
        set_pos(ax1, next_ax1);
        set_pos(ax2, next_ax2);
    }

    int here_ax0 = motors[ax0].microsteps_for_pos(get_pos(ax0));
    int here_ax1 = motors[ax1].microsteps_for_pos(get_pos(ax1));
    int here_ax2 = motors[ax2].microsteps_for_pos(get_pos(ax2));
    float next_ax0 = center_ax0 + radius * cosf(theta1);
    float next_ax1 = center_ax1 + radius * sinf(theta1);
    float next_ax2 = end_ax2;
    int target_ax0 = motors[ax0].microsteps_for_pos(next_ax0);
    int target_ax1 = motors[ax1].microsteps_for_pos(next_ax1);
    int target_ax2 = motors[ax2].microsteps_for_pos(next_ax2); 
    int dpos[] = {
        target_ax0 - here_ax0,
        target_ax1 - here_ax1,
        target_ax2 - here_ax2
    };

    swizzle_linear_step(dpos, rates, ax0, ax1, ax2);
    set_pos(ax0, next_ax0);
    set_pos(ax1, next_ax1);
    set_pos(ax2, next_ax2);


    Serial.print(X); Serial.print(' ');
    Serial.print(Y); Serial.print(' ');
    Serial.println(Z);
    Serial.print(center_ax0 + radius * cosf(theta1)); Serial.print(' ');
    Serial.println(center_ax1 + radius * cosf(theta1));
}

void PathExecutor::swizzle_linear_step(int* d_pos, int* rate, int ax0, int ax1, int ax2) {
    int xyz_dpos[3]{};
    int xyz_rate[3]{};
    xyz_dpos[ax0] = d_pos[0];
    xyz_rate[ax0] = rate[0];
    xyz_dpos[ax1] = d_pos[1];
    xyz_rate[ax1] = rate[1];
    xyz_dpos[ax2] = d_pos[2];
    xyz_rate[ax2] = rate[2];
    linear_step(xyz_dpos[0], xyz_dpos[1], xyz_dpos[2], xyz_rate[0], xyz_rate[1], xyz_rate[2]);
}

void PathExecutor::linear_step(int dx, int dy, int dz, int xrate, int yrate, int zrate) {
    motors[0].set_dir(dx > 0);
    motors[1].set_dir(dy > 0);
    motors[2].set_dir(dz > 0);
    dx = abs(dx);
    dy = abs(dy);
    dz = abs(dz);
    int64_t longest_dist = dx;
    int64_t longest_rate = xrate;
    if(longest_dist*yrate < dy*longest_rate) {
        longest_dist = dy;
        longest_rate = yrate;
    }
    if(longest_dist*zrate < dz*longest_rate) {
        longest_dist = dz;
        longest_rate = zrate;
    }

    int move_time = ceil((float)longest_dist/(float)longest_rate*1000000);
    int x_pos = 0;
    int y_pos = 0;
    int z_pos = 0;

    uint64_t start = micros();
    while(x_pos != dx || y_pos != dy || z_pos != dz) {
        uint64_t dt = micros() - start;
        if(x_pos != dx && (int64_t)dt * (int64_t)dx > (int64_t)x_pos * (int64_t)move_time) {
            x_pos++;
            motors[0].step();
        }
        if(y_pos != dy && (int64_t)dt * (int64_t)dy > (int64_t)y_pos * (int64_t)move_time) {
            y_pos++;
            motors[1].step();
        }
        if(z_pos != dz && (int64_t)dt * (int64_t)dz > (int64_t)z_pos * (int64_t)move_time) {
            z_pos++;
            motors[2].step();
        }
    }
}

void PathExecutor::fast_linear_step(int dx, int dy, int dz) {
    int dpos[3] = {dx,dy,dz};
    int rates[3];
    int accels[3];
    float speeds[3];
    int delays[3];
    uint64_t last[3]{};
    for(int i = 0; i < 3; i++){
        motors[i].set_dir(dpos[i] > 0);
        dpos[i] = abs(dpos[i]);
        rates[i] = motors[i].microsteps_per_sec_fast();
        accels[i] = motors[i].microsteps_per_sec2_fast();
        speeds[i] = 1000;
        delays[i] = 1000000 / speeds[i];
    }

    while(dpos[0] != 0 || dpos[1] != 0 || dpos[2] != 0) {
        uint64_t now = micros();
        for(int i = 0; i < 3; i++){
            if(dpos[i] == 0) continue;
            if(last[i] + delays[i] <= now) {
                motors[i].step();
                dpos[i]--;
                last[i] = now;
                if(speeds[i] * speeds[i] / accels[i] / 2 >= dpos[i]) {
                    speeds[i] -= accels[i] * delays[i] / 1000000;
                    speeds[i] = max(speeds[i], 100.f);
                    delays[i] = 1000000 / speeds[i];
                } else if(speeds[i] >= rates[i]) {
                    speeds[i] = rates[i];
                } else {
                    speeds[i] += accels[i] * delays[i] / 1000000;
                    delays[i] = 1000000 / speeds[i];
                }
            }
        }
    }
}