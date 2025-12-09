import React from 'react';
import '../styles/stLogo.css';
import stLogo from '../../images/stmicroelectronics-logo-2020-white.avif';

const STLogo: React.FC = () => {
  return (
    <div className="st-logo">
      <img
        src={stLogo}
        alt="STMicroelectronics logo"
        className="st-logo-image"
      />
    </div>
  );
};

export default STLogo;
