/*
 *
 * SIXENSE CONFIDENTIAL
 *
 * Copyright (C) 2011 Sixense Entertainment Inc.
 * All Rights Reserved
 *
 */

#ifndef SIXENSE_PORT_UTILS_FPS_HPP
#define SIXENSE_PORT_UTILS_FPS_HPP

#pragma warning(push)
#pragma warning( disable:4251 )

#include "sixense.h"
#include "sixense_math.hpp"
#include "sixense_utils/interfaces.hpp"

#include <vector>
#include <map>

using sixenseMath::Vector2;
using sixenseMath::Vector3;
using sixenseMath::Matrix3;
using sixenseMath::Matrix4;
using sixenseMath::Quat;

#include "export.hpp"

#include "sixense_utils/derivatives.hpp"
#include "sixense_utils/button_states.hpp"
#include "sixense_utils/sixense_utils_string.hpp"

namespace sixenseUtils {

	class SIXENSE_UTILS_EXPORT FPSViewAngles : public IFPSViewAngles {

	public:
		FPSViewAngles();

		void setGame( const char* game_name );

		void setMode( fps_mode mode );
		fps_mode getMode();

		int update( sixenseControllerData *left_cd, sixenseControllerData *right_cd, float frametime_in_ms=0.0f );

		Vector3 getViewAngles(); // The final view angles, ie the feet plus the aiming from the controller
		Vector3 getViewAngleOffset(); // The negative controller aim, used to keep the camera stationary in metroid mode

		Vector3 getSpinSpeed();

		void forceViewAngles( fps_mode mode, Vector3 ); // Used to initialize the view direction when switching into stick spin mode

		void setFeetAnglesMetroid( Vector3 angles );
		Vector3 getFeetAnglesMetroid();

		float getTestVal();

		void setParameter( fps_params param, float val );
		float getParameter( fps_params param );

		void setFov( float hfov, float vfov );
		void getFov( float *hfov, float *vfov );

		void setHoldingTurnSpeed( float horiz, float vert );

		void setRatcheting( bool );
		bool isRatcheting();

		void reset();

		void forceMetroidBlend( float blend_fraction );

	protected:
		Vector2 _feet_angles;
		Vector2 _torso_angles;
		Vector2 _view_offset;

		Vector2 _ratchet_base_angles;

	private:
		fps_mode _mode;

		// Keep track of the game so we can do 
		std::string _game_name;

		// Parameters to control the different modes
		std::vector<float> _param_vals;

		bool _ratcheting;
		bool _just_started_ratcheting, _just_stopped_ratcheting;
	};




	class SIXENSE_UTILS_EXPORT FPSEvents : public IFPSEvents {

	public:

		FPSEvents(); 

		void setGame( const char* game_name );

		void setBinding( fps_event, fps_controller, fps_action, int param );

		void setPointGestureButton( int );

		bool isPointGestureActive();

		int update( sixenseControllerData *left_cd, sixenseControllerData *right_cd, float frametime_in_ms=0.0f );

		bool eventActive( fps_event event_query );
		bool eventStarted( fps_event event_query );
		bool eventStopped( fps_event event_query );

		Vector3 getGrenadeThrowVelocity();

		void setParameter( fps_params param, float val );
		float getParameter( fps_params param );

	private:

		typedef struct {
			fps_event _event;
			fps_controller _controller;
			fps_action _action;
			int _param;
		} fps_binding;

	protected:
		bool testButtonBinding( fps_binding );
		bool testJoystickBinding( fps_binding );
		bool testTriggerBinding( fps_binding );
		bool testTiltBinding( fps_binding );
		bool testPointBinding( fps_binding );
		bool testVelocityBinding( fps_binding );

	private:

		// keep them in a map so there's only one binding per event
		std::map <fps_event, fps_binding> _bindings;

		ButtonStates _button_states[2]; // one for each controller, in order of fps_controller, so 0=left 1=right

		std::vector<bool> _event_started, _event_stopped, _event_persistent_state;
		Derivatives _left_deriv, _right_deriv;
		Derivatives _left_deriv_offset, _right_deriv_offset;

		// Keep track of the velocity of the hand when a grenade is thrown
		Vector3 _grenade_throw_vel;

		// Keep track of the game so we can do 
		std::string _game_name;

		// Parameters to control the different modes
		std::vector<float> _param_vals;

		// The z depth at which one to one mode started. 
		Vector3 _one_to_one_start_pos;

		// This is set when a point gesture is in process
		bool _point_gesture_active;

		// The button that is used to engage point gestures
		unsigned short _point_gesture_button;

	};


	class SIXENSE_UTILS_EXPORT FPSPlayerMovement : public IFPSPlayerMovement{

	public:
		FPSPlayerMovement();

		void setGame( const char* game_name );

		int update( sixenseControllerData *left_cd, sixenseControllerData *right_cd, float frametime_in_ms=0.0f );

		Vector2 getWalkDir();

		void setParameter( fps_movement_params param, float val );
		float getParameter( fps_movement_params param );

	private:
		Vector2 _walk_dir;
		std::string _game_name;

		// Parameters to control the different modes
		std::vector<float> _param_vals;

	};

	class SIXENSE_UTILS_EXPORT FPSMeleeWeapon {
	public:
		FPSMeleeWeapon();

		int update( sixenseControllerData *left_cd, sixenseControllerData *right_cd, float frametime_in_ms=0.0f );

		// Weapon movement
		Vector3 getMeleeWeaponPos();
		Vector3 getMeleeWeaponBladePos();
		Matrix3 getMeleeWeaponMatrix();

		// Attack
		bool swingAttackStarted(); // since last update
		bool isAttacking();
		
		// 1-to-1 mode (when the melee weapon should be moving with the controller and not animated
		bool OneToOneModeStarted();
		bool OneToOneModeStopped();
		bool isInOneToOneMode();

		Vector3 getSwingAttackStartPos();
		Vector3 getSwingAttackDir();

		void setToggleMode( bool mode );

		void forceOneToOneMode( bool mode );

		// Parameters to control the different modes
		typedef enum {
			SWING_START_VELOCITY,
			SWING_STOP_VELOCITY,
			SWING_REARM_VELOCITY,
			BLADE_LENGTH,
			CONTROLLER_ANGLE,

			LEFT_HANDED,

			LAST_MELEE_EVENTS_PARAM
		} melee_params;

		void setParameter( melee_params param, float val );
		float getParameter( melee_params param );


	private:
		Derivatives _left_deriv, _right_deriv;
		bool _armed; // Can we attack again?
		bool _just_started_attack, _attacking;
		bool _just_started_1to1, _just_stopped_1to1, _in_1to1_mode;
		int _swing_wait_count; // How many frames to wait until starting a swing
		int _swing_count; // How long have we been swinging?
		Matrix4 _start_swing_mat;
		Vector3 _swing_dir_vec; // Direction we are attacking in
		Vector3 _swing_start_pos; // Position swing started from

		float _last_trigger_pos;

		Vector3 _weap_pos, _last_weap_pos;
		Matrix3 _weap_mat, _last_weap_mat;

		Vector3 _weap_blade_pos;

		bool _toggle_mode;

		// Parameters to control the different modes
		std::vector<float> _param_vals;

	};

}

#endif

#pragma warning(pop)

