import React from 'react';
import '../styles/butterfly.css';

const Butterfly: React.FC = () => {
  return (
    <div className="butterfly-wrapper" aria-hidden="true">
      <div className="butterfly">
        <div className="wing wing-left" />
        <div className="body" />
        <div className="wing wing-right" />
      </div>
    </div>
  );
};

export default Butterfly;
