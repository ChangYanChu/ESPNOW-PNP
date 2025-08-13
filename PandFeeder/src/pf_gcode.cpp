#include "pf_gcode.h"
#include "pf_servo.h"
#include "pf_config.h"

namespace pf_gcode {

static String inputBuffer;

static float parseParam(char code, float defVal) {
    int pos = inputBuffer.indexOf(code);
    if (pos == -1) return defVal;
    int end = inputBuffer.indexOf(' ', pos + 1);
    if (end == -1) end = inputBuffer.length();
    return inputBuffer.substring(pos + 1, end).toFloat();
}

static void sendAnswer(uint8_t error, const String &msg) {
    if (error == 0) Serial.print(F("ok ")); else Serial.print(F("error "));
    Serial.println(msg);
}

static void processCommand() {
    // Remove comments starting with ';'
    int semicol = inputBuffer.indexOf(';');
    if (semicol >= 0) inputBuffer.remove(semicol);
    inputBuffer.trim();
    if (inputBuffer.length() == 0) return;

    int m = (int)parseParam('M', -1);

    switch (m) {
        case 115: { // M115: report info
            sendAnswer(0, String("PandFeeder ready; channels=") + SERVO_CHANNEL_COUNT);
            break;
        }
        case 17: { // M17: enable
            bool st = pf_servo::setEnabled(true);
            sendAnswer(0, String("enabled=") + (st ? "1" : "0"));
            break;
        }
        case 18: { // M18: disable
            bool st = pf_servo::setEnabled(false);
            sendAnswer(0, String("enabled=") + (st ? "1" : "0"));
            break;
        }
        case 280: { // M280 P<id> S<angle>
            int id = (int)parseParam('P', -1);
            int ang = (int)parseParam('S', -1);
            if (id < 0 || id >= SERVO_CHANNEL_COUNT) { sendAnswer(1, "invalid id"); break; }
            if (ang < SERVO_MIN_ANGLE || ang > SERVO_MAX_ANGLE) { sendAnswer(1, "invalid angle"); break; }
            bool ok = pf_servo::setAngle((uint8_t)id, ang);
            sendAnswer(ok ? 0 : 1, ok ? String("P=") + id + ",S=" + ang : "setAngle failed");
            break;
        }
        case 600: { // M600 N<id> F<len> X<override?> (override unused here)
            int id = (int)parseParam('N', -1);
            int feedLen = (int)parseParam('F', 4);
            if (id < 0 || id >= SERVO_CHANNEL_COUNT) { sendAnswer(1, "invalid id"); break; }
            bool ok = pf_servo::feed((uint8_t)id, (uint8_t)feedLen);
            sendAnswer(ok ? 0 : 1, ok ? String("feed done N=") + id + ",F=" + feedLen : "feed failed");
            break;
        }
        case 601: { // M601 N<id> : set to full retract position
            int id = (int)parseParam('N', -1);
            if (id < 0 || id >= SERVO_CHANNEL_COUNT) { sendAnswer(1, "invalid id"); break; }
            pf_servo::FeederConfig cfg{}; pf_servo::getConfig((uint8_t)id, cfg);
            bool ok = pf_servo::setAngle((uint8_t)id, cfg.retractAngle);
            sendAnswer(ok ? 0 : 1, ok ? String("retract N=") + id : "retract failed");
            break;
        }
        case 602: { // M602 N<id> : report feeder status
            int id = (int)parseParam('N', -1);
            if (id < 0 || id >= SERVO_CHANNEL_COUNT) { sendAnswer(1, "invalid id"); break; }
            pf_servo::FeederStatus st{};
            bool ok = pf_servo::getStatus((uint8_t)id, st);
            if (!ok) { sendAnswer(1, "status failed"); break; }
            String msg = String("N=") + id + ",global=" + (st.globalEnabled?"1":"0") + ",enabled=" + (st.feederEnabled?"1":"0") + ",angle=" + st.lastAngle +
                         ",A=" + st.cfg.fullAdvanceAngle + ",B=" + st.cfg.halfAdvanceAngle + ",C=" + st.cfg.retractAngle + ",F=" + st.cfg.defaultFeedLen +
                         ",U=" + st.cfg.settleTimeMs + ",V=" + st.cfg.minTicks + ",W=" + st.cfg.maxTicks + ",X=" + (st.cfg.ignoreFeedback?"1":"0");
            sendAnswer(0, msg);
            break;
        }
        case 603: { // M603 N<id> A<angle> (default 90 if A omitted)
            int id = (int)parseParam('N', -1);
            int ang = (int)parseParam('A', 90);
            if (id < 0 || id >= SERVO_CHANNEL_COUNT) { sendAnswer(1, "invalid id"); break; }
            if (ang < SERVO_MIN_ANGLE || ang > SERVO_MAX_ANGLE) { sendAnswer(1, "invalid angle"); break; }
            bool ok = pf_servo::setAngle((uint8_t)id, ang);
            sendAnswer(ok ? 0 : 1, ok ? String("N=") + id + ",A=" + ang : "setAngle failed");
            break;
        }
        case 610: { // M610 S0/1 query without S returns state
            int s = (int)parseParam('S', -2);
            if (s == -2) {
                sendAnswer(0, String("enabled=") + (pf_servo::isEnabled() ? "1" : "0"));
            } else {
                if (s != 0 && s != 1) { sendAnswer(1, "invalid S"); break; }
                bool st = pf_servo::setEnabled(s == 1);
                sendAnswer(0, String("enabled=") + (st ? "1" : "0"));
            }
            break;
        }
        case 611: { // M611 N<id?> S0/1 : enable/disable specific feeder or all
            int s = (int)parseParam('S', -1);
            int id = (int)parseParam('N', -1);
            if (s != 0 && s != 1) { sendAnswer(1, "invalid S"); break; }
            if (id == -1) {
                // apply to all
                bool okAll = true;
                for (uint8_t i = 0; i < SERVO_CHANNEL_COUNT; ++i) okAll &= pf_servo::setFeederEnabled(i, s==1);
                sendAnswer(okAll ? 0 : 1, okAll ? String("all feeders enabled=") + s : "partial failure");
            } else {
                if (id < 0 || id >= SERVO_CHANNEL_COUNT) { sendAnswer(1, "invalid id"); break; }
                bool ok = pf_servo::setFeederEnabled((uint8_t)id, s==1);
                sendAnswer(ok ? 0 : 1, ok ? String("N=") + id + ",enabled=" + s : "setFeederEnabled failed");
            }
            break;
        }
        case 620: { // M620 N<id> A B C F U V W X
            int id = (int)parseParam('N', -1);
            if (id < 0 || id >= SERVO_CHANNEL_COUNT) { sendAnswer(1, "invalid id"); break; }
            pf_servo::FeederConfig cfg{}; pf_servo::getConfig((uint8_t)id, cfg);
            int A = (int)parseParam('A', cfg.fullAdvanceAngle);
            int B = (int)parseParam('B', cfg.halfAdvanceAngle);
            int C = (int)parseParam('C', cfg.retractAngle);
            int F = (int)parseParam('F', cfg.defaultFeedLen);
            int U = (int)parseParam('U', cfg.settleTimeMs);
            int V = (int)parseParam('V', cfg.minTicks);
            int W = (int)parseParam('W', cfg.maxTicks);
            int X = (int)parseParam('X', cfg.ignoreFeedback ? 1 : 0);
            // basic validation
            if (A<0||A>180||B<0||B>180||C<0||C>180) { sendAnswer(1, "invalid angle"); break; }
            if (F<2||F>24|| (F%2)!=0) { sendAnswer(1, "invalid F"); break; }
            if (U<50||U>2000) { /* allow wide range */ }
            if (V<1||W<=V) { sendAnswer(1, "invalid V/W"); break; }
            cfg.fullAdvanceAngle=A; cfg.halfAdvanceAngle=B; cfg.retractAngle=C; cfg.defaultFeedLen=(uint8_t)F;
            cfg.settleTimeMs=(uint16_t)U; cfg.minTicks=(uint16_t)V; cfg.maxTicks=(uint16_t)W; cfg.ignoreFeedback=(X!=0);
            bool ok = pf_servo::setConfig((uint8_t)id, cfg);
            sendAnswer(ok ? 0 : 1, ok ? String("cfg saved N=") + id : "cfg save failed");
            break;
        }
        case 621: { // M621: read config for all feeders
            String msg;
            for (uint8_t i=0;i<SERVO_CHANNEL_COUNT;i++) {
                pf_servo::FeederConfig c{}; pf_servo::getConfig(i,c);
                if (i>0) msg += " | ";
                msg += String("N=") + i + ",A=" + c.fullAdvanceAngle + ",B=" + c.halfAdvanceAngle + ",C=" + c.retractAngle + ",F=" + c.defaultFeedLen +
                       ",U=" + c.settleTimeMs + ",V=" + c.minTicks + ",W=" + c.maxTicks + ",X=" + (c.ignoreFeedback?"1":"0");
            }
            sendAnswer(0, msg);
            break;
        }
        default:
            sendAnswer(0, "unknown or empty command ignored");
            break;
    }
}

void injectLine(const String &line) {
    inputBuffer = line;
    processCommand();
    inputBuffer = "";
}

void listen() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        inputBuffer += c;
        if (c == '\n') {
            processCommand();
            inputBuffer = "";
        }
    }
}

} // namespace pf_gcode
